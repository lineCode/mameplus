// license:BSD-3-Clause
// copyright-holders:Manuel Abadia
/***************************************************************************

    Blades of Steel (GX797) (c) 1987 Konami

    Driver by Manuel Abadia <emumanu+mame@gmail.com>

    Interrupts:

        CPU #0 (6309):
        --------------
        * IRQ: not used.
        * FIRQ: generated by VBLANK.
        * NMI: writes the sound command to the 6809.

        CPU #1 (6809):
        --------------
        * IRQ: triggered by the 6309 when a sound command is written.
        * FIRQ: not used.
        * NMI: not used.

    Notes:
        * The protection is not fully understood(Konami 051733). The
        game is playable, but is not 100% accurate.
        * Missing samples.
        (both issues above are outdated?)

***************************************************************************/

#include "emu.h"
#include "includes/bladestl.h"
#include "includes/konamipt.h"

#include "cpu/m6809/m6809.h"
#include "cpu/m6809/hd6309.h"
#include "machine/watchdog.h"
#include "sound/2203intf.h"
#include "screen.h"
#include "speaker.h"


TIMER_DEVICE_CALLBACK_MEMBER(bladestl_state::bladestl_scanline)
{
	int scanline = param;

	if(scanline == 240 && m_k007342->is_int_enabled()) // vblank-out irq
		m_maincpu->set_input_line(HD6309_FIRQ_LINE, HOLD_LINE);

	if(scanline == 0) // vblank-in or timer irq
		m_maincpu->set_input_line(INPUT_LINE_NMI, PULSE_LINE);
}

/*************************************
 *
 *  Memory handlers
 *
 *************************************/

READ8_MEMBER(bladestl_state::trackball_r)
{
	int curr = m_trackball[offset]->read();
	int delta = (curr - m_last_track[offset]) & 0xff;
	m_last_track[offset] = curr;

	return (delta & 0x80) | (curr >> 1);
}

WRITE8_MEMBER(bladestl_state::bladestl_bankswitch_w)
{
	/* bits 0 & 1 = coin counters */
	machine().bookkeeping().coin_counter_w(0,data & 0x01);
	machine().bookkeeping().coin_counter_w(1,data & 0x02);

	/* bits 2 & 3 = lamps */
	output().set_led_value(0,data & 0x04);
	output().set_led_value(1,data & 0x08);

	/* bit 4 = relay (???) */

	/* bits 5-6 = bank number */
	m_rombank->set_entry((data & 0x60) >> 5);

	/* bit 7 = select sprite bank */
	m_spritebank = (data & 0x80) << 3;

}

WRITE8_MEMBER(bladestl_state::bladestl_port_B_w)
{
	// bits 3-5 = ROM bank select
	m_upd7759->set_bank_base(((data & 0x38) >> 3) * 0x20000);

	// bit 2 = SSG-C rc filter enable
	m_filter3->filter_rc_set_RC(filter_rc_device::LOWPASS, 1000, 2200, 1000, data & 0x04 ? CAP_N(150) : 0); /* YM2203-SSG-C */

	// bit 1 = SSG-B rc filter enable
	m_filter2->filter_rc_set_RC(filter_rc_device::LOWPASS, 1000, 2200, 1000, data & 0x02 ? CAP_N(150) : 0); /* YM2203-SSG-B */

	// bit 0 = SSG-A rc filter enable
	m_filter1->filter_rc_set_RC(filter_rc_device::LOWPASS, 1000, 2200, 1000, data & 0x01 ? CAP_N(150) : 0); /* YM2203-SSG-A */
}

READ8_MEMBER(bladestl_state::bladestl_speech_busy_r)
{
	return m_upd7759->busy_r() ? 1 : 0;
}

WRITE8_MEMBER(bladestl_state::bladestl_speech_ctrl_w)
{
	m_upd7759->reset_w(data & 1);
	m_upd7759->start_w(data & 2);
}

/*************************************
 *
 *  Address maps
 *
 *************************************/

void bladestl_state::main_map(address_map &map)
{
	map(0x0000, 0x1fff).rw(m_k007342, FUNC(k007342_device::read), FUNC(k007342_device::write));    /* Color RAM + Video RAM */
	map(0x2000, 0x21ff).rw(m_k007420, FUNC(k007420_device::read), FUNC(k007420_device::write));    /* Sprite RAM */
	map(0x2200, 0x23ff).rw(m_k007342, FUNC(k007342_device::scroll_r), FUNC(k007342_device::scroll_w));  /* Scroll RAM */
	map(0x2400, 0x245f).ram().w("palette", FUNC(palette_device::write_indirect)).share("palette");  /* palette */
	map(0x2600, 0x2607).w(m_k007342, FUNC(k007342_device::vreg_w));          /* Video Registers */
	map(0x2e00, 0x2e00).portr("COINSW");             /* DIPSW #3, coinsw, startsw */
	map(0x2e01, 0x2e01).portr("P1");                 /* 1P controls */
	map(0x2e02, 0x2e02).portr("P2");                 /* 2P controls */
	map(0x2e03, 0x2e03).portr("DSW2");               /* DISPW #2 */
	map(0x2e40, 0x2e40).portr("DSW1");               /* DIPSW #1 */
	map(0x2e80, 0x2e80).w("soundlatch", FUNC(generic_latch_8_device::write)); /* cause interrupt on audio CPU */
	map(0x2ec0, 0x2ec0).w("watchdog", FUNC(watchdog_timer_device::reset_w));
	map(0x2f00, 0x2f03).r(this, FUNC(bladestl_state::trackball_r));               /* Trackballs */
	map(0x2f40, 0x2f40).w(this, FUNC(bladestl_state::bladestl_bankswitch_w));    /* bankswitch control */
	map(0x2f80, 0x2f9f).rw("k051733", FUNC(k051733_device::read), FUNC(k051733_device::write));    /* Protection: 051733 */
	map(0x2fc0, 0x2fc0).nopw();                        /* ??? */
	map(0x4000, 0x5fff).ram();                             /* Work RAM */
	map(0x6000, 0x7fff).bankr("rombank");              /* banked ROM */
	map(0x8000, 0xffff).rom();
}

void bladestl_state::sound_map(address_map &map)
{
	map(0x0000, 0x07ff).ram();
	map(0x1000, 0x1001).rw("ymsnd", FUNC(ym2203_device::read), FUNC(ym2203_device::write));    /* YM2203 */
	map(0x3000, 0x3000).w(this, FUNC(bladestl_state::bladestl_speech_ctrl_w));   /* UPD7759 */
	map(0x4000, 0x4000).r(this, FUNC(bladestl_state::bladestl_speech_busy_r));    /* UPD7759 */
	map(0x5000, 0x5000).w("soundlatch", FUNC(generic_latch_8_device::acknowledge_w));
	map(0x6000, 0x6000).r("soundlatch", FUNC(generic_latch_8_device::read));
	map(0x8000, 0xffff).rom();
}


/*************************************
 *
 *  Input ports
 *
 *************************************/

static INPUT_PORTS_START( bladestl_joy ) // Joystick set does not even have a Trackball test
	PORT_START("DSW1")
	KONAMI_COINAGE_ALT_LOC(SW1)

	PORT_START("DSW2")
	PORT_DIPUNUSED_DIPLOC( 0x01, 0x01, "SW2:1" )
	PORT_DIPUNUSED_DIPLOC( 0x02, 0x02, "SW2:2" )
	PORT_DIPNAME( 0x04, 0x00, DEF_STR( Cabinet ) )      PORT_DIPLOCATION("SW2:3")
	PORT_DIPSETTING(    0x00, DEF_STR( Upright ) )
	PORT_DIPSETTING(    0x04, DEF_STR( Cocktail ) )
	PORT_DIPNAME( 0x18, 0x18, "Bonus time set" )        PORT_DIPLOCATION("SW2:4,5")
	PORT_DIPSETTING(    0x18, "30 secs" )
	PORT_DIPSETTING(    0x10, "20 secs" )
	PORT_DIPSETTING(    0x08, "15 secs" )
	PORT_DIPSETTING(    0x00, "10 secs" )
	PORT_DIPNAME( 0x60, 0x40, DEF_STR( Difficulty ) )   PORT_DIPLOCATION("SW2:6,7")
	PORT_DIPSETTING(    0x60, DEF_STR( Easy ) )
	PORT_DIPSETTING(    0x40, DEF_STR( Normal ) )
	PORT_DIPSETTING(    0x20, DEF_STR( Difficult ) )
	PORT_DIPSETTING(    0x00, DEF_STR( Very_Difficult ) )
	PORT_DIPNAME( 0x80, 0x00, DEF_STR( Demo_Sounds ) )  PORT_DIPLOCATION("SW2:8")
	PORT_DIPSETTING(    0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )

	PORT_START("COINSW")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_COIN3 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x20, 0x20, DEF_STR( Flip_Screen ) )  PORT_DIPLOCATION("SW3:1")
	PORT_DIPSETTING(    0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(    0x00, DEF_STR( On ) )
	PORT_DIPUNUSED_DIPLOC( 0x40, 0x40, "SW3:2" )
	PORT_SERVICE_DIPLOC(  0x80, IP_ACTIVE_LOW, "SW3:3" )

	PORT_START("P1")
	KONAMI8_B123(1)
	PORT_DIPNAME( 0x80, 0x80, "Period time set" )       PORT_DIPLOCATION("SW3:4")
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x00, "7" )

	PORT_START("P2")
	KONAMI8_B123_UNK(2)

	PORT_START("TRACKBALL.0")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("TRACKBALL.1")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("TRACKBALL.2")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START("TRACKBALL.3")
	PORT_BIT( 0xff, IP_ACTIVE_LOW, IPT_UNUSED )
INPUT_PORTS_END

static INPUT_PORTS_START( bladestl_track ) // Joystick inputs are still read in test mode, but not used
	PORT_INCLUDE( bladestl_joy )

	PORT_MODIFY("DSW2")
	PORT_DIPUNUSED_DIPLOC( 0x08, 0x08, "SW2:4" )    /* Listed as "Unused" */
	PORT_DIPUNUSED_DIPLOC( 0x10, 0x10, "SW2:5" )    /* Listed as "Unused" */

	PORT_MODIFY("P1")
	PORT_DIPUNUSED_DIPLOC( 0x80, 0x80, "SW3:4" )    /* Listed as "Unused" */

	PORT_MODIFY("TRACKBALL.0")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(63) PORT_REVERSE PORT_PLAYER(1)

	PORT_MODIFY("TRACKBALL.1")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(63) PORT_PLAYER(1)

	PORT_MODIFY("TRACKBALL.2")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_X ) PORT_SENSITIVITY(100) PORT_KEYDELTA(63) PORT_PLAYER(2)

	PORT_MODIFY("TRACKBALL.3")
	PORT_BIT( 0xff, 0x00, IPT_TRACKBALL_Y ) PORT_SENSITIVITY(100) PORT_KEYDELTA(63) PORT_PLAYER(2)
INPUT_PORTS_END



/*************************************
 *
 *  Graphics definitions
 *
 *************************************/

static const gfx_layout charlayout =
{
	8,8,            /* 8 x 8 characters */
	0x40000/32,     /* 8192 characters */
	4,              /* 4bpp */
	{ 0, 1, 2, 3 }, /* the four bitplanes are packed in one nibble */
	{ 2*4, 3*4, 0*4, 1*4, 6*4, 7*4, 4*4, 5*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8            /* every character takes 32 consecutive bytes */
};

static const gfx_layout spritelayout =
{
	8,8,            /* 8*8 sprites */
	0x40000/32,     /* 8192 sprites */
	4,              /* 4 bpp */
	{ 0, 1, 2, 3 }, /* the four bitplanes are packed in one nibble */
	{ 0*4, 1*4, 2*4, 3*4, 4*4, 5*4, 6*4, 7*4 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8            /* every sprite takes 32 consecutive bytes */
};

static GFXDECODE_START( bladestl )
	GFXDECODE_ENTRY( "gfx1", 0, charlayout,    0,  2 ) /* colors 00..31 */
	GFXDECODE_ENTRY( "gfx2", 0, spritelayout, 32, 16 ) /* colors 32..47 but using lookup table */
GFXDECODE_END


/*************************************
 *
 *  Machine driver
 *
 *************************************/

void bladestl_state::machine_start()
{
	m_rombank->configure_entries(0, 4, memregion("maincpu")->base(), 0x2000);

	save_item(NAME(m_spritebank));
	save_item(NAME(m_last_track));
}

void bladestl_state::machine_reset()
{
	int i;

	m_spritebank = 0;

	for (i = 0; i < 4 ; i++)
		m_last_track[i] = 0;

	m_soundlatch->acknowledge_w(machine().dummy_space(), 0, 0);
}

MACHINE_CONFIG_START(bladestl_state::bladestl)

	/* basic machine hardware */
	MCFG_DEVICE_ADD(m_maincpu, HD6309E, XTAL(24'000'000) / 8) // divider not verified (from 007342 custom)
	MCFG_DEVICE_PROGRAM_MAP(main_map)
	MCFG_TIMER_DRIVER_ADD_SCANLINE("scantimer", bladestl_state, bladestl_scanline, "screen", 0, 1)

	MCFG_DEVICE_ADD(m_audiocpu, MC6809E, XTAL(24'000'000) / 16)
	MCFG_DEVICE_PROGRAM_MAP(sound_map)

	MCFG_QUANTUM_TIME(attotime::from_hz(600))

	MCFG_WATCHDOG_ADD("watchdog")

	/* video hardware */
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_REFRESH_RATE(60)
	MCFG_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(0))
	MCFG_SCREEN_SIZE(32*8, 32*8)
	MCFG_SCREEN_VISIBLE_AREA(0*8, 32*8-1, 2*8, 30*8-1)
	MCFG_SCREEN_UPDATE_DRIVER(bladestl_state, screen_update_bladestl)
	MCFG_SCREEN_PALETTE("palette")

	MCFG_GFXDECODE_ADD(m_gfxdecode, "palette", bladestl)
	MCFG_PALETTE_ADD("palette", 32 + 16*16)
	MCFG_PALETTE_INDIRECT_ENTRIES(32+16)
	MCFG_PALETTE_FORMAT(xBBBBBGGGGGRRRRR)
	MCFG_PALETTE_INIT_OWNER(bladestl_state, bladestl)

	MCFG_K007342_ADD(m_k007342)
	MCFG_K007342_GFXNUM(0)
	MCFG_K007342_CALLBACK_OWNER(bladestl_state, bladestl_tile_callback)
	MCFG_K007342_GFXDECODE("gfxdecode")

	MCFG_K007420_ADD(m_k007420)
	MCFG_K007420_BANK_LIMIT(0x3ff)
	MCFG_K007420_CALLBACK_OWNER(bladestl_state, bladestl_sprite_callback)
	MCFG_K007420_PALETTE("palette")

	MCFG_K051733_ADD("k051733")

	/* sound hardware */
	/* the initialization order is important, the port callbacks being
	   called at initialization time */
	SPEAKER(config, "mono").front_center();

	MCFG_GENERIC_LATCH_8_ADD(m_soundlatch)
	MCFG_GENERIC_LATCH_DATA_PENDING_CB(INPUTLINE(m_audiocpu, M6809_IRQ_LINE))
	MCFG_GENERIC_LATCH_SEPARATE_ACKNOWLEDGE(true)

	MCFG_DEVICE_ADD(m_upd7759, UPD7759)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.60)

	MCFG_DEVICE_ADD("ymsnd", YM2203, XTAL(24'000'000) / 8)
	MCFG_AY8910_PORT_A_WRITE_CB(WRITE8(m_upd7759, upd775x_device, port_w))
	MCFG_AY8910_PORT_B_WRITE_CB(WRITE8(*this, bladestl_state, bladestl_port_B_w))
	MCFG_SOUND_ROUTE(0, "filter1", 0.45)
	MCFG_SOUND_ROUTE(1, "filter2", 0.45)
	MCFG_SOUND_ROUTE(2, "filter3", 0.45)
	MCFG_SOUND_ROUTE(3, "mono", 0.45)

	MCFG_DEVICE_ADD(m_filter1, FILTER_RC)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MCFG_DEVICE_ADD(m_filter2, FILTER_RC)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
	MCFG_DEVICE_ADD(m_filter3, FILTER_RC)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.0)
MACHINE_CONFIG_END


/*************************************
 *
 *  ROM definition(s)
 *
 *************************************/

ROM_START( bladestl )
	ROM_REGION( 0x10000, "maincpu", 0 ) /* code + banked roms */
	ROM_LOAD( "797-t01.19c", 0x00000, 0x10000, CRC(89d7185d) SHA1(0d2f346d9515cab0389106c0e227fb0bd84a2c9c) )

	ROM_REGION( 0x10000, "audiocpu", 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "797-c02.12d", 0x08000, 0x08000, CRC(65a331ea) SHA1(f206f6c5f0474542a5b7686b2f4d2cc7077dd5b9) )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "797a05.19h", 0x00000, 0x40000, CRC(5491ba28) SHA1(c807774827c55c211ab68f548e1e835289cc5744) )   /* tiles */

	ROM_REGION( 0x40000, "gfx2", 0 )
	ROM_LOAD( "797a06.13h", 0x00000, 0x40000, CRC(d055f5cc) SHA1(3723b39b2a3e6dd8e7fc66bbfe1eef9f80818774) )   /* sprites */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "797a07.16i", 0x0000, 0x0100, CRC(7aecad4e) SHA1(05150a8dd25bdd6ab0c5b350e6ffd272f040e46a) ) /* sprites lookup table, 63S141N BPROM */

	ROM_REGION( 0xc0000, "upd", 0 ) /* uPD7759 data (chip 1) */
	ROM_LOAD( "797a03.11a", 0x00000, 0x80000, CRC(9ee1a542) SHA1(c9a142a326875a50f03e49e83a84af8bb423a467) )
	ROM_LOAD( "797a04.9a",  0x80000, 0x40000, CRC(9ac8ea4e) SHA1(9f81eff970c9e8aea6f67d8a7d89805fae044ae1) )
ROM_END

ROM_START( bladestll )
	ROM_REGION( 0x10000, "maincpu", 0 ) /* code + banked roms */
	ROM_LOAD( "797-l01.19c", 0x00000, 0x10000, CRC(1ab14c40) SHA1(c566e31a666b467d75f5fc9fa427986c3ebc705c) )

	ROM_REGION( 0x10000, "audiocpu", 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "797-c02.12d", 0x08000, 0x08000, CRC(65a331ea) SHA1(f206f6c5f0474542a5b7686b2f4d2cc7077dd5b9) )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "797a05.19h", 0x00000, 0x40000, CRC(5491ba28) SHA1(c807774827c55c211ab68f548e1e835289cc5744) )   /* tiles */

	ROM_REGION( 0x40000, "gfx2", 0 )
	ROM_LOAD( "797a06.13h", 0x00000, 0x40000, CRC(d055f5cc) SHA1(3723b39b2a3e6dd8e7fc66bbfe1eef9f80818774) )   /* sprites */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "797a07.16i", 0x0000, 0x0100, CRC(7aecad4e) SHA1(05150a8dd25bdd6ab0c5b350e6ffd272f040e46a) ) /* sprites lookup table, 63S141N BPROM */

	ROM_REGION( 0xc0000, "upd", 0 ) /* uPD7759 data (chip 1) */
	ROM_LOAD( "797a03.11a", 0x00000, 0x80000, CRC(9ee1a542) SHA1(c9a142a326875a50f03e49e83a84af8bb423a467) )
	ROM_LOAD( "797a04.9a",  0x80000, 0x40000, CRC(9ac8ea4e) SHA1(9f81eff970c9e8aea6f67d8a7d89805fae044ae1) )
ROM_END

ROM_START( bladestle )
	ROM_REGION( 0x10000, "maincpu", 0 ) /* code + banked roms */
	ROM_LOAD( "797-e01.19c", 0x00000, 0x10000, CRC(f8472e95) SHA1(8b6caa905fb1642300dd9da508871b00429872c3) )

	ROM_REGION( 0x10000, "audiocpu", 0 ) /* 64k for the sound CPU */
	ROM_LOAD( "797-c02.12d", 0x08000, 0x08000, CRC(65a331ea) SHA1(f206f6c5f0474542a5b7686b2f4d2cc7077dd5b9) )

	ROM_REGION( 0x40000, "gfx1", 0 )
	ROM_LOAD( "797a05.19h", 0x00000, 0x40000, CRC(5491ba28) SHA1(c807774827c55c211ab68f548e1e835289cc5744) )   /* tiles */

	ROM_REGION( 0x40000, "gfx2", 0 )
	ROM_LOAD( "797a06.13h", 0x00000, 0x40000, CRC(d055f5cc) SHA1(3723b39b2a3e6dd8e7fc66bbfe1eef9f80818774) )   /* sprites */

	ROM_REGION( 0x0100, "proms", 0 )
	ROM_LOAD( "797a07.16i", 0x0000, 0x0100, CRC(7aecad4e) SHA1(05150a8dd25bdd6ab0c5b350e6ffd272f040e46a) ) /* sprites lookup table, 63S141N BPROM */

	ROM_REGION( 0xc0000, "upd", 0 ) /* uPD7759 data (chip 1) */
	ROM_LOAD( "797a03.11a", 0x00000, 0x80000, CRC(9ee1a542) SHA1(c9a142a326875a50f03e49e83a84af8bb423a467) )
	ROM_LOAD( "797a04.9a",  0x80000, 0x40000, CRC(9ac8ea4e) SHA1(9f81eff970c9e8aea6f67d8a7d89805fae044ae1) )
ROM_END


/*************************************
 *
 *  Game driver(s)
 *
 *************************************/

GAME( 1987, bladestl,  0,        bladestl, bladestl_joy,   bladestl_state, 0, ROT90, "Konami", "Blades of Steel (version T, Joystick)", MACHINE_SUPPORTS_SAVE )
GAME( 1987, bladestll, bladestl, bladestl, bladestl_track, bladestl_state, 0, ROT90, "Konami", "Blades of Steel (version L, Trackball)", MACHINE_SUPPORTS_SAVE )
GAME( 1987, bladestle, bladestl, bladestl, bladestl_track, bladestl_state, 0, ROT90, "Konami", "Blades of Steel (version E, Trackball)", MACHINE_SUPPORTS_SAVE )
