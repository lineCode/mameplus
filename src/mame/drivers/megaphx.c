/* 
   Dinamic / Inder arcade hardware

   Mega Phoenix
 
 also known to exist on this hardware:
   Hammer Boy
   Nonamed 2 (ever finished? only code seen has 1991 date and is vastly incomplete) (versions exist for Amstrad CPC, MSX and Spectrum)
   After The War



  trivia: Test mode graphics are the same as Little Robin(?!), TMS is very similar too, suggesting they share a common codebase.


 ToDo:
  - where should roms 6/7 map, they contain the 68k vectors, but the game expects RAM at 0, and it doesn't seem to read any of the other data from those roms.. they contain
    a cross hatch pattern amongst other things?
 Sound:
  - how does banking work? when the irq callbacks happen for each irq level? currently no way to access this because it's a daisy chain setup with the ctc?
  - even if i hack that the title screen speech doesn't work properly - is there a timing register like little robin?
 I/O:
  - port_c_r / port_c_w should go through the 8255 but I don't see how to hook them up that way? various bits of the writes are lost?

  
  --


  Chips of note

  Main board:

  TS68000CP8
  TMS34010FNL-40
  TMP82C55AP-2
 
  Bt478KPJ35  Palette / RAMDAC

  Actel A1010A-PL68C  (custom blitter maybe?)

  2x 8 DSW, bottom corner, away from everything..

 Sub / Sound board:

  ST Z8430AB1

  custom INDER badged chip 40 pin?  (probably just a z80 - it's in the sound section)
	MODELO: MEGA PHOENIX
	KIT NO. 1.034
	FECHA FABRICACION 08.10.91
	LA MANIPULCION DE LA ETIQUETA O DE LA PLACA ANULA SU SARANTIA
	(this sticker is also present on the other PCB)


*/

#include "emu.h"
#include "cpu/tms34010/tms34010.h"
#include "cpu/m68000/m68000.h"


#include "video/ramdac.h"
#include "machine/i8255.h"
#include "machine/inder_sb.h"



class megaphx_state : public driver_device
{
public:
	megaphx_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_mainram(*this, "mainram"),
		m_vram(*this, "vram"),
		port_c_value(0),
		m_palette(*this, "palette"),
		m_tms(*this, "tms"),
		m_indersb(*this, "inder_sb")

	{ 
		m_shiftfull = 0;

	}

	required_device<cpu_device> m_maincpu;
	required_shared_ptr<UINT16> m_mainram;
	required_shared_ptr<UINT16> m_vram;





	DECLARE_DRIVER_INIT(megaphx);
	DECLARE_MACHINE_RESET(megaphx);






	DECLARE_READ8_MEMBER(port_c_r);
	DECLARE_WRITE8_MEMBER(port_c_w);

	int m_pic_is_reset;
	int m_pic_shift_pos;
	int m_pic_data;
	int m_pic_data_bit;
	int m_pic_clock;
	int m_pic_readbit;

	UINT16 m_pic_result;

	UINT8 port_c_value;
	required_device<palette_device> m_palette;
	required_device<tms34010_device> m_tms;
	required_device<inder_sb_device> m_indersb;


	int m_shiftfull; // this might be a driver specific hack for a TMS bug.




};






static ADDRESS_MAP_START( megaphx_68k_map, AS_PROGRAM, 16, megaphx_state )
	AM_RANGE(0x000000, 0x0013ff) AM_RAM AM_SHARE("mainram") // maps over part of the rom??

	AM_RANGE(0x000000, 0x03ffff) AM_ROM AM_REGION("roms67", 0x00000) // or the rom doesn't map here? it contains the service mode grid amongst other things..

	AM_RANGE(0x040000, 0x040007) AM_DEVREADWRITE("tms", tms34010_device, host_r, host_w)

	AM_RANGE(0x050000, 0x050001) AM_DEVWRITE("inder_sb", inder_sb_device, megaphx_0x050000_w)
	AM_RANGE(0x050002, 0x050003) AM_DEVREAD("inder_sb", inder_sb_device, megaphx_0x050002_r)


	AM_RANGE(0x060004, 0x060005) AM_READ8( port_c_r, 0x00ff )
	AM_RANGE(0x060006, 0x060007) AM_WRITE8( port_c_w, 0x00ff )
	AM_RANGE(0x060000, 0x060003) AM_DEVREADWRITE8("ppi8255_0", i8255_device, read, write, 0x00ff)
	
	AM_RANGE(0x800000, 0x83ffff) AM_ROM  AM_REGION("roms01", 0x00000) // code + bg gfx are in here
	AM_RANGE(0x840000, 0x87ffff) AM_ROM  AM_REGION("roms23", 0x00000) // bg gfx are in here
	AM_RANGE(0x880000, 0x8bffff) AM_ROM  AM_REGION("roms45", 0x00000) // bg gfx + title screen in here

ADDRESS_MAP_END


static ADDRESS_MAP_START( megaphx_tms_map, AS_PROGRAM, 16, megaphx_state )

	AM_RANGE(0x00000000, 0x003fffff) AM_RAM AM_SHARE("vram") // vram?
//	AM_RANGE(0x00100000, 0x002fffff) AM_RAM  // vram?
//	AM_RANGE(0x00300000, 0x003fffff) AM_RAM
//	AM_RANGE(0x04000000, 0x040000ff) AM_WRITENOP

	AM_RANGE(0x04000000, 0x0400000f) AM_DEVWRITE8("ramdac",ramdac_device,index_w,0x00ff)
	AM_RANGE(0x04000010, 0x0400001f) AM_DEVREADWRITE8("ramdac",ramdac_device,pal_r,pal_w,0x00ff)
	AM_RANGE(0x04000030, 0x0400003f) AM_DEVWRITE8("ramdac",ramdac_device,index_r_w,0x00ff)
	AM_RANGE(0x04000090, 0x0400009f) AM_WRITENOP

	AM_RANGE(0xc0000000, 0xc00001ff) AM_DEVREADWRITE("tms", tms34010_device, io_register_r, io_register_w)
	AM_RANGE(0xffc00000, 0xffffffff) AM_RAM
ADDRESS_MAP_END


static void megaphx_scanline(screen_device &screen, bitmap_rgb32 &bitmap, int scanline, const tms34010_display_params *params)
{
	megaphx_state *state = screen.machine().driver_data<megaphx_state>();

	UINT16 *vram = &state->m_vram[(params->rowaddr << 8) & 0x3ff00];
	UINT32 *dest = &bitmap.pix32(scanline);

	const pen_t *paldata = state->m_palette->pens();

	int coladdr = params->coladdr;
	int x;

	for (x = params->heblnk; x < params->hsblnk; x += 2)
	{
		UINT16 pixels = vram[coladdr++ & 0xff];
		dest[x + 0] = paldata[pixels & 0xff];
		dest[x + 1] = paldata[pixels >> 8];
	}

}


static void megaphx_to_shiftreg(address_space &space, UINT32 address, UINT16 *shiftreg)
{
	megaphx_state *state = space.machine().driver_data<megaphx_state>();

	if (state->m_shiftfull == 0)
	{
		//printf("read to shift regs address %08x (%08x)\n", address, TOWORD(address) * 2);

		memcpy(shiftreg, &state->m_vram[TOWORD(address) & ~TOWORD(0x1fff)], TOBYTE(0x2000)); // & ~TOWORD(0x1fff) is needed for round 6
		state->m_shiftfull = 1;
	}
}

static void megaphx_from_shiftreg(address_space &space, UINT32 address, UINT16 *shiftreg)
{
//	printf("write from shift regs address %08x (%08x)\n", address, TOWORD(address) * 2);

	megaphx_state *state = space.machine().driver_data<megaphx_state>();
	memcpy(&state->m_vram[TOWORD(address) & ~TOWORD(0x1fff)], shiftreg, TOBYTE(0x2000));

	state->m_shiftfull = 0;
}

MACHINE_RESET_MEMBER(megaphx_state,megaphx)
{
}

static void m68k_gen_int(device_t *device, int state)
{
	megaphx_state *drvstate = device->machine().driver_data<megaphx_state>();
	if (state) drvstate->m_maincpu->set_input_line(4, ASSERT_LINE);
	else drvstate->m_maincpu->set_input_line(4, CLEAR_LINE);

}


static const tms34010_config tms_config_megaphx =
{
	TRUE,                          /* halt on reset */
	"screen",                       /* the screen operated on */
	XTAL_40MHz/12,                   /* pixel clock */
	2,                              /* pixels per clock */
	NULL,                           /* scanline callback (indexed16) */
	megaphx_scanline,              /* scanline callback (rgb32) */
	m68k_gen_int,                   /* generate interrupt */
	megaphx_to_shiftreg,           /* write to shiftreg function */
	megaphx_from_shiftreg          /* read from shiftreg function */
};



static INPUT_PORTS_START( megaphx )
	PORT_START("P0") // verified in test mode
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(1) // shield
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(1) // unused ? (in test mode)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(1) // high score entry
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(1) // high score entry
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(1)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(1)

	PORT_START("P1") // verified in test mode
	PORT_BIT( 0x0001, IP_ACTIVE_LOW, IPT_BUTTON2 ) PORT_PLAYER(2) // shield
	PORT_BIT( 0x0002, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_BUTTON3 ) PORT_PLAYER(2) // unused ? (in test mode)
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_JOYSTICK_UP ) PORT_PLAYER(2) // high score entry
	PORT_BIT( 0x0010, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN ) PORT_PLAYER(2) //high score entry
	PORT_BIT( 0x0020, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT ) PORT_PLAYER(2)
	PORT_BIT( 0x0040, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2)
	PORT_BIT( 0x0080, IP_ACTIVE_LOW, IPT_BUTTON1 ) PORT_PLAYER(2)


	PORT_START("PIC1") // via PIC  (check the other bits aren't used for anything, eg. screen timing for sound playback like little robin)
	PORT_DIPNAME( 0x0001, 0x0001, "XX" )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0002, 0x0002, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_BIT( 0x0004, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x0008, IP_ACTIVE_LOW, IPT_START2 )
	PORT_DIPNAME( 0x0010, 0x0010, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0020, 0x0020, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, DEF_STR( Unknown ) )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )

	PORT_START("DSW1") // via PIC
	PORT_DIPNAME( 0x0007, 0x0003, DEF_STR( Coin_A ) )
	PORT_DIPSETTING(      0x0007, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0006, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0005, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0004, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0003, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0002, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x0038, 0x0018, DEF_STR( Coin_B ) )
	PORT_DIPSETTING(      0x0038, DEF_STR( 5C_1C ) )
	PORT_DIPSETTING(      0x0030, DEF_STR( 4C_1C ) )
	PORT_DIPSETTING(      0x0028, DEF_STR( 3C_1C ) )
	PORT_DIPSETTING(      0x0020, DEF_STR( 2C_1C ) )
	PORT_DIPSETTING(      0x0018, DEF_STR( 1C_1C ) )
	PORT_DIPSETTING(      0x0010, DEF_STR( 1C_2C ) )
	PORT_DIPSETTING(      0x0008, DEF_STR( 1C_3C ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( 1C_4C ) )
	PORT_DIPNAME( 0x00c0, 0x0080, DEF_STR( Lives ) )
	PORT_DIPSETTING(      0x0000, "1" )
	PORT_DIPSETTING(      0x0040, "2" )
	PORT_DIPSETTING(      0x0080, "3" )
	PORT_DIPSETTING(      0x00c0, "4" )


	PORT_START("DSW2") // via PIC  // some of these are difficulty
	PORT_DIPNAME( 0x0001, 0x0000, DEF_STR( Demo_Sounds ) )
	PORT_DIPSETTING(      0x0001, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_SERVICE( 0x0002, IP_ACTIVE_HIGH ) 
	PORT_DIPNAME( 0x001c, 0x0010, "Difficulty?"  ) // in hammer boy at least..
	PORT_DIPSETTING(      0x0000, "0" )
	PORT_DIPSETTING(      0x0004, "1" )
	PORT_DIPSETTING(      0x0008, "2" )
	PORT_DIPSETTING(      0x000c, "3" )
	PORT_DIPSETTING(      0x0010, "4" )
	PORT_DIPSETTING(      0x0014, "5" )
	PORT_DIPSETTING(      0x0018, "6" )
	PORT_DIPSETTING(      0x001c, "7" )
	PORT_DIPNAME( 0x0020, 0x0020, "DSW2-20" ) // something to do with time in hammer boy??
	PORT_DIPSETTING(      0x0020, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0040, 0x0040, "DSW2-40" )
	PORT_DIPSETTING(      0x0040, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
	PORT_DIPNAME( 0x0080, 0x0080, "DSW2-80" )
	PORT_DIPSETTING(      0x0080, DEF_STR( Off ) )
	PORT_DIPSETTING(      0x0000, DEF_STR( On ) )
INPUT_PORTS_END

static ADDRESS_MAP_START( ramdac_map, AS_0, 8, megaphx_state )
	AM_RANGE(0x000, 0x3ff) AM_DEVREADWRITE("ramdac",ramdac_device,ramdac_pal_r,ramdac_rgb888_w)
ADDRESS_MAP_END

static RAMDAC_INTERFACE( ramdac_intf )
{
	1
};

/* why don't the port_c read/writes work properly when hooked through the 8255? */

// the PIC is accessed serially through clock / data lines, each time 16-bits are accessed..
// not 100% sure if the command takes effect after all 16-bits are written, or after 8..

READ8_MEMBER(megaphx_state::port_c_r)
{
	
	//printf("read port c - write value was %02x\n", port_c_value);

//	int pc = machine().device("maincpu")->safe_pc();
	UINT8 ret = 0;

//	printf("(%06x) port_c_r (thru 8255)\n", pc);
	
	if (m_pic_clock == 1) ret |= 0x08;
	if (m_pic_readbit == 1) ret |= 0x02;
//	return ioport("SYS")->read();
	return ret;
}


WRITE8_MEMBER(megaphx_state::port_c_w)
{
	


//	int pc = machine().device("maincpu")->safe_pc();
	port_c_value = (data & 0x0f);

	if (port_c_value == 0x9)
	{
	//	printf("Assert PIC reset line\n");
		m_pic_is_reset = 1;
	}
	else if (port_c_value == 0x8)
	{
	//	printf("Clear PIC reset line\n");
		m_pic_is_reset = 0;
	
		m_pic_shift_pos = 0;
		m_pic_data = 0;
		m_pic_data_bit = 0;
		m_pic_readbit = 0;
		m_pic_clock = 1;

	}
	else if (port_c_value == 0xd)
	{
	//	printf("Set PIC data line\n");
		m_pic_data_bit = 1;
	}
	else if (port_c_value == 0xc)
	{
	//	printf("Clear PIC data line\n");
		m_pic_data_bit = 0;
	}
	else if (port_c_value == 0xf)
	{
		if (m_pic_clock == 0)
		{
		//	printf("Set PIC clock line | pos %d | bit %d\n", m_pic_shift_pos, m_pic_data_bit);
			




			m_pic_clock = 1;
		
		}
	}
	else if (port_c_value == 0xe)
	{

		if (m_pic_clock == 1)
		{
			m_pic_data |= m_pic_data_bit << m_pic_shift_pos;

			if (m_pic_shift_pos == 8)
			{
				//printf("------------------ sending command %02x\n", m_pic_data);

				if (m_pic_data == 0xfe) // get software version??
				{
					m_pic_result = (ioport("PIC1")->read()) | (0XFF << 8);
				}
				else if (m_pic_data == 0x82) // dsw1
				{
					m_pic_result = (ioport("PIC1")->read()) | ((ioport("DSW1")->read()) << 8);
				}
				else if (m_pic_data == 0x86) // dsw2
				{
					m_pic_result = (ioport("PIC1")->read()) | ((ioport("DSW2")->read()) << 8);
				}
				else
				{
					printf("unknown PIC command %02x\n", m_pic_data);
				}
			}

			m_pic_readbit = (m_pic_result >> (m_pic_shift_pos)) & 1;


			m_pic_shift_pos++;


			//	printf("Clear PIC clock line\n");
			m_pic_clock = 0;
		}
	}
	else
	{
	//	printf("Unknown write to PIC %02x (PC %06x)\n", port_c_value, pc);
	}



}


static I8255A_INTERFACE( ppi8255_intf_0 )
{
	DEVCB_INPUT_PORT("P0"),        /* Port A read */
	DEVCB_NULL,                     /* Port A write */
	DEVCB_INPUT_PORT("P1"),        /* Port B read */
	DEVCB_NULL,                     /* Port B write */
	DEVCB_NULL,        /* Port C read */ // should be connected to above functions but values are incorrect
	DEVCB_NULL,        /* Port C write */  // should be connected to above functions but values are incorrect
};



static MACHINE_CONFIG_START( megaphx, megaphx_state )

	MCFG_CPU_ADD("maincpu", M68000, 8000000) // ??  can't read xtal due to reflections, CPU is an 8Mhz part
	MCFG_CPU_PROGRAM_MAP(megaphx_68k_map)

	MCFG_CPU_ADD("tms", TMS34010, XTAL_40MHz)
	MCFG_CPU_CONFIG(tms_config_megaphx)
	MCFG_CPU_PROGRAM_MAP(megaphx_tms_map)

	MCFG_INDER_AUDIO_ADD("inder_sb")

	MCFG_I8255A_ADD( "ppi8255_0", ppi8255_intf_0 )

	MCFG_MACHINE_RESET_OVERRIDE(megaphx_state,megaphx)

	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_RAW_PARAMS(XTAL_40MHz/12, 424, 0, 338-1, 262, 0, 246-1)
	MCFG_SCREEN_UPDATE_DEVICE("tms", tms34010_device, tms340x0_rgb32)

	MCFG_PALETTE_ADD("palette", 256)
	
	MCFG_RAMDAC_ADD("ramdac", ramdac_intf, ramdac_map, "palette")
MACHINE_CONFIG_END

DRIVER_INIT_MEMBER(megaphx_state,megaphx)
{
	UINT16 *src = (UINT16*)memregion( "roms67" )->base();
	// copy vector table? - it must be writable because the game write the irq vector..
	memcpy(m_mainram, src, 0x80);


}


ROM_START( megaphx )
	ROM_REGION16_BE( 0x40000, "roms67", 0 )  // the majority of the data in these does not get used?! (only the vector table) is it just garbage??
	ROM_LOAD16_BYTE( "mph6.u32", 0x000001, 0x20000, CRC(b99703d4) SHA1(393b6869e71d4c61060e66e0e9e36a1e6ca345d1) )
	ROM_LOAD16_BYTE( "mph7.u21", 0x000000, 0x20000, CRC(f11e7449) SHA1(1017142d10011d68e49d3ccdb1ac4e815c03b17a) )

	ROM_REGION16_BE( 0x40000, "roms01", 0 )
	ROM_LOAD16_BYTE( "mph0.u38", 0x000001, 0x20000, CRC(b63dd20f) SHA1(c8ce5985a6ba49428d66a49d9d623ccdfce422c2) )
	ROM_LOAD16_BYTE( "mph1.u27", 0x000000, 0x20000, CRC(4dcbf44b) SHA1(a8fa49ecd033f1aeb323e0032ddcf5f8f9463ac0) )

	ROM_REGION16_BE( 0x40000, "roms23", 0 )
	ROM_LOAD16_BYTE( "mph2.u37", 0x000001, 0x20000, CRC(a0f69c27) SHA1(d0c5c241d94a1f03f51e7e517e2f9dec6abcf75a) )
	ROM_LOAD16_BYTE( "mph3.u26", 0x000000, 0x20000, CRC(4db84cc5) SHA1(dd74acd4b32c7e7553554ac0f9ba13503358e869) )

	ROM_REGION16_BE( 0x40000, "roms45", 0 )
	ROM_LOAD16_BYTE( "mph4.u36", 0x000001, 0x20000, CRC(c8e0725e) SHA1(b3af315b9a94a692e81e0dbfd4035036c2af4f50) )
	ROM_LOAD16_BYTE( "mph5.u25", 0x000000, 0x20000, CRC(c95ccb69) SHA1(9d14cbfafd943f6ff461a7f373170a35e36eb695) )

	ROM_REGION( 0x200000, "inder_sb:user2", 0 )
	ROM_LOAD( "sonido_mph1.u39", 0x00000, 0x20000, CRC(f5e65557) SHA1(5ae759c2bcef96fbda42f088c02b6dec208030f3) )
	ROM_LOAD( "sonido_mph2.u38", 0x20000, 0x20000, CRC(7444d0f9) SHA1(9739b48993bccea5530533b67808d13d6155ffe3) )

	ROM_REGION( 0x100000, "inder_sb:audiocpu", 0 )
	ROM_LOAD( "sonido_mph0.u35", 0x000000, 0x2000,  CRC(abc1b140) SHA1(8384a162d85cf9ea870d22f44b1ca64001c6a083) )

	ROM_REGION( 0x100000, "pals", 0 ) // jedutil won't convert these? are they bad?
	ROM_LOAD( "p31_u31_palce16v8h-25.jed", 0x000, 0xbd4, CRC(05ef04b7) SHA1(330dd81a832b6675fb0473868c26fe9bec2da854) )
	ROM_LOAD( "p40_u29_palce16v8h-25.jed", 0x000, 0xbd4, CRC(44b7e51c) SHA1(b8b34f3b319d664ec3ad72ed87d9f65701f183a5) )

	// there is a PIC responsible for some I/O tasks (what type? what internal rom size?)
ROM_END

GAME( 1991, megaphx,  0,        megaphx, megaphx, megaphx_state, megaphx, ROT0, "Dinamic / Inder", "Mega Phoenix", GAME_IMPERFECT_GRAPHICS | GAME_IMPERFECT_SOUND )
