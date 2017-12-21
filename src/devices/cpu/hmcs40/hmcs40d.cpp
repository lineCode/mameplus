// license:BSD-3-Clause
// copyright-holders:hap
/*

  Hitachi HMCS40 MCU family disassembler

  NOTE: start offset(basepc) is $3F, not 0

*/

#include "emu.h"
#include "hmcs40d.h"

const char *const hmcs40_disassembler::s_mnemonics[] =
{
	"?",
	"LAB", "LBA", "LAY", "LASPX", "LASPY", "XAMR",
	"LXA", "LYA", "LXI", "LYI", "IY", "DY", "AYY", "SYY", "XSP",
	"LAM", "LBM", "XMA", "XMB", "LMAIY", "LMADY",
	"LMIIY", "LAI", "LBI",
	"AI", "IB", "DB", "AMC", "SMC", "AM", "DAA", "DAS", "NEGA", "COMB", "SEC", "REC", "TC", "ROTL", "ROTR", "OR",
	"MNEI", "YNEI", "ANEM", "BNEM", "ALEI", "ALEM", "BLEM",
	"SEM", "REM", "TM",
	"BR", "CAL", "LPU", "TBR", "RTN",
	"SEIE", "SEIF0", "SEIF1", "SETF", "SECF", "REIE", "REIF0", "REIF1", "RETF", "RECF", "TI0", "TI1", "TIF0", "TIF1", "TTF", "LTI", "LTA", "LAT", "RTNI",
	"SED", "RED", "TD", "SEDD", "REDD", "LAR", "LBR", "LRA", "LRB", "P",
	"NOP"
};

// number of bits per opcode parameter, 99 means (XY) parameter, negative means reversed bit-order
const s8 hmcs40_disassembler::s_bits[] =
{
	0,
	0, 0, 0, 0, 0, 4,
	0, 0, -4, -4, 0, 0, 0, 0, 99,
	99, 99, 99, 99, 99, 99,
	-4, -4, -4,
	-4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	-4, -4, 0, 0, -4, 0, 0,
	2, 2, 2,
	6, 6, 5, 3, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -4, 0, 0, 0,
	0, 0, 0, 4, 4, 3, 3, 3, 3, 3,
	0
};

const u32 hmcs40_disassembler::s_flags[] =
{
	0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0,
	0, 0, 0,
	0, STEP_OVER, 0, 0, STEP_OUT,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, STEP_OUT,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0
};

const u8 hmcs40_disassembler::hmcs40_mnemonic[0x400] =
{
/*  0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F  */
	/* 0x000 */
	mNOP,  mXSP,  mXSP,  mXSP,  mSEM,  mSEM,  mSEM,  mSEM,  mLAM,  mLAM,  mLAM,  mLAM,  0,     0,     0,     0,
	mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,mLMIIY,
	mLBM,  mLBM,  mLBM,  mLBM,  mBLEM, 0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	mAMC,  0,     0,     0,     mAM,   0,     0,     0,     0,     0,     0,     0,     mLTA,  0,     0,     0,
	/* 0x040 */
	mLXA,  0,     0,     0,     0,     mDAS,  mDAA,  0,     0,     0,     0,     0,     mREC,  0,     0,     mSEC,
	mLYA,  0,     0,     0,     mIY,   0,     0,     0,     mAYY,  0,     0,     0,     0,     0,     0,     0,
	mLBA,  0,     0,     0,     mIB,   0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,  mLAI,
	/* 0x080 */
	mAI,   mAI,   mAI,   mAI,   mAI,   mAI,   mAI,   mAI,   mAI,   mAI,   mAI,   mAI,   mAI,   mAI,   mAI,   mAI,
	mSED,  0,     0,     0,     mTD,   0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	mSEIF1,mSECF, mSEIF0,0,     mSEIE, mSETF, 0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	/* 0x0c0 */
	mLAR,  mLAR,  mLAR,  mLAR,  mLAR,  mLAR,  mLAR,  mLAR,  0,     0,     0,     0,     0,     0,     0,     0,
	mSEDD, mSEDD, mSEDD, mSEDD, 0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	mLBR,  mLBR,  mLBR,  mLBR,  mLBR,  mLBR,  mLBR,  mLBR,  0,     0,     0,     0,     0,     0,     0,     0,
	mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR, mXAMR,

/*  0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F  */
	/* 0x100 */
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	mLMAIY,mLMAIY,0,     0,     mLMADY,mLMADY,0,     0,     mLAY,  0,     0,     0,     0,     0,     0,     0,
	mOR,   0,     0,     0,     mANEM, 0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	/* 0x140 */
	mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,  mLXI,
	mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,  mLYI,
	mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,  mLBI,
	mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,  mLTI,
	/* 0x180 */
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	mTIF1, mTI1,  mTIF0, mTI0,  0,     mTTF,  0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	/* 0x1c0 */
	mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,
	mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,
	mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,
	mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,   mBR,

/*  0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F  */
	/* 0x200 */
	mTM,   mTM,   mTM,   mTM,   mREM,  mREM,  mREM,  mREM,  mXMA,  mXMA,  mXMA,  mXMA,  0,     0,     0,     0,
	mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI, mMNEI,
	mXMB,  mXMB,  mXMB,  mXMB,  mROTR, mROTL, 0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	mSMC,  0,     0,     0,     mALEM, 0,     0,     0,     0,     0,     0,     0,     mLAT,  0,     0,     0,
	/* 0x240 */
	mLASPX,0,     0,     0,     mNEGA, 0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     mTC,
	mLASPY,0,     0,     0,     mDY,   0,     0,     0,     mSYY,  0,     0,     0,     0,     0,     0,     0,
	mLAB,  0,     0,     0,     0,     0,     0,     mDB,   0,     0,     0,     0,     0,     0,     0,     0,
	mALEI, mALEI, mALEI, mALEI, mALEI, mALEI, mALEI, mALEI, mALEI, mALEI, mALEI, mALEI, mALEI, mALEI, mALEI, mALEI,
	/* 0x280 */
	mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI, mYNEI,
	mRED,  0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	mREIF1,mRECF, mREIF0,0,     mREIE, mRETF, 0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	/* 0x2c0 */
	mLRA,  mLRA,  mLRA,  mLRA,  mLRA,  mLRA,  mLRA,  mLRA,  0,     0,     0,     0,     0,     0,     0,     0,
	mREDD, mREDD, mREDD, mREDD, 0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	mLRB,  mLRB,  mLRB,  mLRB,  mLRB,  mLRB,  mLRB,  mLRB,  0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,

/*  0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F  */
	/* 0x300 */
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	mCOMB, 0,     0,     0,     mBNEM, 0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	/* 0x340 */
	mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,
	mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,  mLPU,
	mTBR,  mTBR,  mTBR,  mTBR,  mTBR,  mTBR,  mTBR,  mTBR,  mP,    mP,    mP,    mP,    mP,    mP,    mP,    mP,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	/* 0x380 */
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     mRTNI, 0,     0,     mRTN,  0,     0,     0,     0,     0,     0,     0,     0,
	0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
	/* 0x3c0 */
	mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,
	mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,
	mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,
	mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL,  mCAL
};



offs_t hmcs40_disassembler::disassemble(std::ostream &stream, offs_t pc, const data_buffer &opcodes, const data_buffer &params)
{
	u16 op = opcodes.r16(pc) & 0x3ff;
	u8 instr = hmcs40_mnemonic[op];
	s8 bits = s_bits[instr];

	// special case for (XY) opcode
	if (bits == 99)
	{
		util::stream_format(stream, "%s", s_mnemonics[instr]);

		if (op & 1)
			stream << "X";
		if (op & 2)
			stream << "Y";
	}
	else
	{
		util::stream_format(stream, "%-6s ", s_mnemonics[instr]);

		// opcode parameter
		if (bits != 0)
		{
			u8 param = op;

			// reverse bits
			if (bits < 0)
			{
				param = bitswap<8>(param,0,1,2,3,4,5,6,7);
				param >>= (8 + bits);
				bits = -bits;
			}

			param &= ((1 << bits) - 1);

			if (bits > 5)
				util::stream_format(stream, "$%02X", param);
			else
				util::stream_format(stream, "%d", param);
		}
	}

	return 1 | s_flags[instr] | SUPPORTED;
}

u32 hmcs40_disassembler::opcode_alignment() const
{
	return 1;
}

u32 hmcs40_disassembler::interface_flags() const
{
	return NONLINEAR_PC | PAGED;
}

u32 hmcs40_disassembler::page_address_bits() const
{
	return 6;
}

offs_t hmcs40_disassembler::pc_linear_to_real(offs_t pc) const
{
	static const u8 l2r[64] = {
		0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x3e, 0x3d, 0x3b, 0x37, 0x2f, 0x1e, 0x3c, 0x39, 0x33,
		0x27, 0x0e, 0x1d, 0x3a, 0x35, 0x2b, 0x16, 0x2c, 0x18, 0x30, 0x21, 0x02, 0x05, 0x0b, 0x17, 0x2e,
		0x1c, 0x38, 0x31, 0x23, 0x06, 0x0d, 0x1b, 0x36, 0x2d, 0x1a, 0x34, 0x29, 0x12, 0x24, 0x08, 0x11,
		0x22, 0x04, 0x09, 0x13, 0x26, 0x0c, 0x19, 0x32, 0x25, 0x0a, 0x15, 0x2a, 0x14, 0x28, 0x10, 0x20,
	};
	return (pc & ~0x3f) | l2r[pc & 0x3f];
}

offs_t hmcs40_disassembler::pc_real_to_linear(offs_t pc) const
{
	static const u8 r2l[64] = {
		0x00, 0x01, 0x1b, 0x02, 0x31, 0x1c, 0x24, 0x03, 0x2e, 0x32, 0x39, 0x1d, 0x35, 0x25, 0x11, 0x04,
		0x3e, 0x2f, 0x2c, 0x33, 0x3c, 0x3a, 0x16, 0x1e, 0x18, 0x36, 0x29, 0x26, 0x20, 0x12, 0x0c, 0x05,
		0x3f, 0x1a, 0x30, 0x23, 0x2d, 0x38, 0x34, 0x10, 0x3d, 0x2b, 0x3b, 0x15, 0x17, 0x28, 0x1f, 0x0b,
		0x19, 0x22, 0x37, 0x0f, 0x2a, 0x14, 0x27, 0x0a, 0x21, 0x0e, 0x13, 0x09, 0x0d, 0x08, 0x07, 0x06,
	};
	return (pc & ~0x3f) | r2l[pc & 0x3f];
}

