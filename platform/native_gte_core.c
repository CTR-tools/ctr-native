/*
 * Derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/src/gte/PsyX_GTE.cpp
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#include <macros.h>
#include <psx/gtereg.h>
#include <psx/libgte.h>

GTERegisters gteRegs;

#define GTE_SF(op)    ((op >> 19) & 1)
#define GTE_MX(op)    ((op >> 17) & 3)
#define GTE_V(op)     ((op >> 15) & 3)
#define GTE_CV(op)    ((op >> 13) & 3)
#define GTE_LM(op)    ((op >> 10) & 1)
#define GTE_FUNCT(op) (op & 63)

#define gteop(code)   (code & 0x1ffffff)

#define VX(n)         (n < 3 ? gteRegs.CP2D.p[n << 1].sw.l : C2_IR1)
#define VY(n)         (n < 3 ? gteRegs.CP2D.p[n << 1].sw.h : C2_IR2)
#define VZ(n)         (n < 3 ? gteRegs.CP2D.p[(n << 1) + 1].sw.l : C2_IR3)
#define MX11(n)       (n < 3 ? gteRegs.CP2C.p[(n << 3)].sw.l : -C2_R << 4)
#define MX12(n)       (n < 3 ? gteRegs.CP2C.p[(n << 3)].sw.h : C2_R << 4)
#define MX13(n)       (n < 3 ? gteRegs.CP2C.p[(n << 3) + 1].sw.l : C2_IR0)
#define MX21(n)       (n < 3 ? gteRegs.CP2C.p[(n << 3) + 1].sw.h : C2_R13)
#define MX22(n)       (n < 3 ? gteRegs.CP2C.p[(n << 3) + 2].sw.l : C2_R13)
#define MX23(n)       (n < 3 ? gteRegs.CP2C.p[(n << 3) + 2].sw.h : C2_R13)
#define MX31(n)       (n < 3 ? gteRegs.CP2C.p[(n << 3) + 3].sw.l : C2_R22)
#define MX32(n)       (n < 3 ? gteRegs.CP2C.p[(n << 3) + 3].sw.h : C2_R22)
#define MX33(n)       (n < 3 ? gteRegs.CP2C.p[(n << 3) + 4].sw.l : C2_R22)
#define CV1(n)        (n < 3 ? gteRegs.CP2C.p[(n << 3) + 5].sd : 0)
#define CV2(n)        (n < 3 ? gteRegs.CP2C.p[(n << 3) + 6].sd : 0)
#define CV3(n)        (n < 3 ? gteRegs.CP2C.p[(n << 3) + 7].sd : 0)

global_variable int m_sf;
global_variable s64 m_mac0;
global_variable s64 m_mac3;

u32 gte_leadingzerocount(u32 lzcs)
{
	u32 lzcr = ((s32)lzcs < 0) ? ~lzcs : lzcs;
	local_persist const char debruijn32[32] = {0, 31, 9, 30, 3, 8,  13, 29, 2,  5,  7,  21, 12, 24, 28, 19,
	                                           1, 10, 4, 14, 6, 22, 25, 20, 11, 15, 23, 26, 16, 27, 17, 18};

	if (!lzcr)
	{
		return 32;
	}

	lzcr |= lzcr >> 1;
	lzcr |= lzcr >> 2;
	lzcr |= lzcr >> 4;
	lzcr |= lzcr >> 8;
	lzcr |= lzcr >> 16;
	lzcr++;

	return debruijn32[lzcr * 0x076be629 >> 27];
}

int LIM(int value, int max, int min, unsigned int flag)
{
	if (value > max)
	{
		C2_FLAG |= flag;
		return max;
	}
	else if (value < min)
	{
		C2_FLAG |= flag;
		return min;
	}

	return value;
}

internal inline s64 gte_shift(s64 a, int sf)
{
	if (sf > 0)
	{
		return a >> 12;
	}
	else if (sf < 0)
	{
		return a << 12;
	}

	return a;
}

internal int BOUNDS(/*int44*/ s64 value, int max_flag, int min_flag)
{
	if (value /*.positive_overflow()*/ > (s64)0x7ffffffffff)
	{
		C2_FLAG |= max_flag;
	}

	if (value /*.negative_overflow()*/ < (s64)-0x8000000000)
	{
		C2_FLAG |= min_flag;
	}

	return (int)(gte_shift(value /*.value()*/, m_sf));
}

internal u32 gte_divide(u16 numerator, u16 denominator)
{
	if (numerator < (denominator * 2))
	{
		local_persist const u8 table[] = {
		    0xff, 0xfd, 0xfb, 0xf9, 0xf7, 0xf5, 0xf3, 0xf1, 0xef, 0xee, 0xec, 0xea, 0xe8, 0xe6, 0xe4, 0xe3, 0xe1, 0xdf, 0xdd, 0xdc, 0xda, 0xd8, 0xd6, 0xd5,
		    0xd3, 0xd1, 0xd0, 0xce, 0xcd, 0xcb, 0xc9, 0xc8, 0xc6, 0xc5, 0xc3, 0xc1, 0xc0, 0xbe, 0xbd, 0xbb, 0xba, 0xb8, 0xb7, 0xb5, 0xb4, 0xb2, 0xb1, 0xb0,
		    0xae, 0xad, 0xab, 0xaa, 0xa9, 0xa7, 0xa6, 0xa4, 0xa3, 0xa2, 0xa0, 0x9f, 0x9e, 0x9c, 0x9b, 0x9a, 0x99, 0x97, 0x96, 0x95, 0x94, 0x92, 0x91, 0x90,
		    0x8f, 0x8d, 0x8c, 0x8b, 0x8a, 0x89, 0x87, 0x86, 0x85, 0x84, 0x83, 0x82, 0x81, 0x7f, 0x7e, 0x7d, 0x7c, 0x7b, 0x7a, 0x79, 0x78, 0x77, 0x75, 0x74,
		    0x73, 0x72, 0x71, 0x70, 0x6f, 0x6e, 0x6d, 0x6c, 0x6b, 0x6a, 0x69, 0x68, 0x67, 0x66, 0x65, 0x64, 0x63, 0x62, 0x61, 0x60, 0x5f, 0x5e, 0x5d, 0x5d,
		    0x5c, 0x5b, 0x5a, 0x59, 0x58, 0x57, 0x56, 0x55, 0x54, 0x53, 0x53, 0x52, 0x51, 0x50, 0x4f, 0x4e, 0x4d, 0x4d, 0x4c, 0x4b, 0x4a, 0x49, 0x48, 0x48,
		    0x47, 0x46, 0x45, 0x44, 0x43, 0x43, 0x42, 0x41, 0x40, 0x3f, 0x3f, 0x3e, 0x3d, 0x3c, 0x3c, 0x3b, 0x3a, 0x39, 0x39, 0x38, 0x37, 0x36, 0x36, 0x35,
		    0x34, 0x33, 0x33, 0x32, 0x31, 0x31, 0x30, 0x2f, 0x2e, 0x2e, 0x2d, 0x2c, 0x2c, 0x2b, 0x2a, 0x2a, 0x29, 0x28, 0x28, 0x27, 0x26, 0x26, 0x25, 0x24,
		    0x24, 0x23, 0x22, 0x22, 0x21, 0x20, 0x20, 0x1f, 0x1e, 0x1e, 0x1d, 0x1d, 0x1c, 0x1b, 0x1b, 0x1a, 0x19, 0x19, 0x18, 0x18, 0x17, 0x16, 0x16, 0x15,
		    0x15, 0x14, 0x14, 0x13, 0x12, 0x12, 0x11, 0x11, 0x10, 0x0f, 0x0f, 0x0e, 0x0e, 0x0d, 0x0d, 0x0c, 0x0c, 0x0b, 0x0a, 0x0a, 0x09, 0x09, 0x08, 0x08,
		    0x07, 0x07, 0x06, 0x06, 0x05, 0x05, 0x04, 0x04, 0x03, 0x03, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00};

		int shift = gte_leadingzerocount(denominator) - 16;

		int r1 = (denominator << shift) & 0x7fff;
		int r2 = table[((r1 + 0x40) >> 7)] + 0x101;
		int r3 = ((0x80 - (r2 * (r1 + 0x8000))) >> 8) & 0x1ffff;
		u32 reciprocal = ((r2 * r3) + 0x80) >> 8;

		return (u32)((((u64)reciprocal * (numerator << shift)) + 0x8000) >> 16);
	}

	return 0xffffffff;
}

/* Setting bits 12 & 19-22 in FLAG does not set bit 31 */

internal int A1(/*int44*/ s64 a)
{
	return BOUNDS(a, (1 << 31) | (1 << 30), (1 << 31) | (1 << 27));
}
internal int A2(/*int44*/ s64 a)
{
	return BOUNDS(a, (1 << 31) | (1 << 29), (1 << 31) | (1 << 26));
}
internal int A3(/*int44*/ s64 a)
{
	m_mac3 = a;
	return BOUNDS(a, (1 << 31) | (1 << 28), (1 << 31) | (1 << 25));
}
internal int Lm_B1(int a, int lm)
{
	return LIM(a, 0x7fff, -0x8000 * !lm, (1 << 31) | (1 << 24));
}
internal int Lm_B2(int a, int lm)
{
	return LIM(a, 0x7fff, -0x8000 * !lm, (1 << 31) | (1 << 23));
}
internal int Lm_B3(int a, int lm)
{
	return LIM(a, 0x7fff, -0x8000 * !lm, (1 << 22));
}

internal int Lm_B3_sf(s64 value, int sf, int lm)
{
	int value_sf = (int)(gte_shift(value, sf));
	int value_12 = (int)(gte_shift(value, 1));
	int max = 0x7fff;
	int min = 0;
	if (lm == 0)
	{
		min = -0x8000;
	}

	if (value_12 < -0x8000 || value_12 > 0x7fff)
	{
		C2_FLAG |= (1 << 22);
	}

	if (value_sf > max)
	{
		return max;
	}
	else if (value_sf < min)
	{
		return min;
	}

	return value_sf;
}

internal int Lm_C1(int a)
{
	return LIM(a, 0x00ff, 0x0000, (1 << 21));
}
internal int Lm_C2(int a)
{
	return LIM(a, 0x00ff, 0x0000, (1 << 20));
}
internal int Lm_C3(int a)
{
	return LIM(a, 0x00ff, 0x0000, (1 << 19));
}
internal int Lm_D(s64 a, int sf)
{
	return LIM((int)(gte_shift(a, sf)), 0xffff, 0x0000, (1 << 31) | (1 << 18));
}

internal u32 Lm_E(u32 result)
{
	if (result == 0xffffffff)
	{
		C2_FLAG |= (1 << 31) | (1 << 17);
		return 0x1ffff;
	}

	if (result > 0x1ffff)
	{
		return 0x1ffff;
	}

	return result;
}

internal s64 F(s64 a)
{
	m_mac0 = a;

	if (a > 0x7fffffffLL)
	{
		C2_FLAG |= (1 << 31) | (1 << 16);
	}

	if (a < -0x80000000LL)
	{
		C2_FLAG |= (1 << 31) | (1 << 15);
	}

	return a;
}

internal int Lm_G1(s64 a)
{
	if (a > 0x3ff)
	{
		C2_FLAG |= (1 << 31) | (1 << 14);
		return 0x3ff;
	}
	if (a < -0x400)
	{
		C2_FLAG |= (1 << 31) | (1 << 14);
		return -0x400;
	}

	return (int)(a);
}

internal int Lm_G2(s64 a)
{
	if (a > 0x3ff)
	{
		C2_FLAG |= (1 << 31) | (1 << 13);
		return 0x3ff;
	}

	if (a < -0x400)
	{
		C2_FLAG |= (1 << 31) | (1 << 13);
		return -0x400;
	}

	return (int)(a);
}

internal int Lm_G1_ia(s64 a)
{
	if (a > 0x3ffffff)
	{
		return 0x3ffffff;
	}

	if (a < -0x4000000)
	{
		return -0x4000000;
	}

	return (int)(a);
}

internal int Lm_G2_ia(s64 a)
{
	if (a > 0x3ffffff)
	{
		return 0x3ffffff;
	}

	if (a < -0x4000000)
	{
		return -0x4000000;
	}

	return (int)(a);
}

internal int Lm_H(s64 value, int sf)
{
	s64 value_sf = gte_shift(value, sf);
	int value_12 = (int)(gte_shift(value, 1));
	int max = 0x1000;
	int min = 0x0000;

	if (value_sf < min || value_sf > max)
	{
		C2_FLAG |= (1 << 12);
	}

	if (value_12 > max)
	{
		return max;
	}

	if (value_12 < min)
	{
		return min;
	}

	return value_12;
}

internal int GTE_RotTransPers(int idx, int lm)
{
	int h_over_sz3;

	C2_MAC1 = A1(/*int44*/ (s64)((s64)C2_TRX << 12) + (C2_R11 * VX(idx)) + (C2_R12 * VY(idx)) + (C2_R13 * VZ(idx)));
	C2_MAC2 = A2(/*int44*/ (s64)((s64)C2_TRY << 12) + (C2_R21 * VX(idx)) + (C2_R22 * VY(idx)) + (C2_R23 * VZ(idx)));
	C2_MAC3 = A3(/*int44*/ (s64)((s64)C2_TRZ << 12) + (C2_R31 * VX(idx)) + (C2_R32 * VY(idx)) + (C2_R33 * VZ(idx)));
	C2_IR1 = Lm_B1(C2_MAC1, lm);
	C2_IR2 = Lm_B2(C2_MAC2, lm);
	C2_IR3 = Lm_B3_sf(m_mac3, m_sf, lm);
	C2_SZ0 = C2_SZ1;
	C2_SZ1 = C2_SZ2;
	C2_SZ2 = C2_SZ3;
	C2_SZ3 = Lm_D(m_mac3, 1);
	h_over_sz3 = Lm_E(gte_divide(C2_H, C2_SZ3));
	C2_SXY0 = C2_SXY1;
	C2_SXY1 = C2_SXY2;
	C2_SX2 = Lm_G1(F((s64)C2_OFX + ((s64)C2_IR1 * h_over_sz3)) >> 16);
	C2_SY2 = Lm_G2(F((s64)C2_OFY + ((s64)C2_IR2 * h_over_sz3)) >> 16);

	return h_over_sz3;
}

int GTE_operator(int op)
{
	int v;
	int lm;
	int cv;
	int mx;
	int h_over_sz3 = 0;

	lm = GTE_LM(gteop(op));
	m_sf = GTE_SF(gteop(op));

	C2_FLAG = 0;

	switch (GTE_FUNCT(gteop(op)))
	{
	case 0x00:
	case 0x01:
		h_over_sz3 = GTE_RotTransPers(0, lm);

		C2_MAC0 = (int)(F((s64)C2_DQB + ((s64)C2_DQA * h_over_sz3)));
		C2_IR0 = Lm_H(m_mac0, 1);

		return 1;

	case 0x06:
		C2_MAC0 = (int)(F((s64)(C2_SX0 * C2_SY1) + (C2_SX1 * C2_SY2) + (C2_SX2 * C2_SY0) - (C2_SX0 * C2_SY2) - (C2_SX1 * C2_SY0) - (C2_SX2 * C2_SY1)));
		C2_FLAG = 0;
		return 1;

	case 0x0c:

		C2_MAC1 = A1((s64)(C2_R22 * C2_IR3) - (C2_R33 * C2_IR2));
		C2_MAC2 = A2((s64)(C2_R33 * C2_IR1) - (C2_R11 * C2_IR3));
		C2_MAC3 = A3((s64)(C2_R11 * C2_IR2) - (C2_R22 * C2_IR1));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		return 1;

	case 0x10:

		C2_MAC1 = A1((C2_R << 16) + (C2_IR0 * Lm_B1(A1(((s64)C2_RFC << 12) - (C2_R << 16)), 0)));
		C2_MAC2 = A2((C2_G << 16) + (C2_IR0 * Lm_B2(A2(((s64)C2_GFC << 12) - (C2_G << 16)), 0)));
		C2_MAC3 = A3((C2_B << 16) + (C2_IR0 * Lm_B3(A3(((s64)C2_BFC << 12) - (C2_B << 16)), 0)));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_RGB0 = C2_RGB1;
		C2_RGB1 = C2_RGB2;
		C2_CD2 = C2_CODE;
		C2_R2 = Lm_C1(C2_MAC1 >> 4);
		C2_G2 = Lm_C2(C2_MAC2 >> 4);
		C2_B2 = Lm_C3(C2_MAC3 >> 4);
		return 1;

	case 0x11:

		C2_MAC1 = A1((C2_IR1 << 12) + (C2_IR0 * Lm_B1(A1(((s64)C2_RFC << 12) - (C2_IR1 << 12)), 0)));
		C2_MAC2 = A2((C2_IR2 << 12) + (C2_IR0 * Lm_B2(A2(((s64)C2_GFC << 12) - (C2_IR2 << 12)), 0)));
		C2_MAC3 = A3((C2_IR3 << 12) + (C2_IR0 * Lm_B3(A3(((s64)C2_BFC << 12) - (C2_IR3 << 12)), 0)));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_RGB0 = C2_RGB1;
		C2_RGB1 = C2_RGB2;
		C2_CD2 = C2_CODE;
		C2_R2 = Lm_C1(C2_MAC1 >> 4);
		C2_G2 = Lm_C2(C2_MAC2 >> 4);
		C2_B2 = Lm_C3(C2_MAC3 >> 4);
		return 1;

	case 0x12:

		mx = GTE_MX(gteop(op));
		v = GTE_V(gteop(op));
		cv = GTE_CV(gteop(op));

		switch (cv)
		{
		case 2:
			C2_MAC1 = A1((s64)(MX12(mx) * VY(v)) + (MX13(mx) * VZ(v)));
			C2_MAC2 = A2((s64)(MX22(mx) * VY(v)) + (MX23(mx) * VZ(v)));
			C2_MAC3 = A3((s64)(MX32(mx) * VY(v)) + (MX33(mx) * VZ(v)));
			Lm_B1(A1(((s64)CV1(cv) << 12) + (MX11(mx) * VX(v))), 0);
			Lm_B2(A2(((s64)CV2(cv) << 12) + (MX21(mx) * VX(v))), 0);
			Lm_B3(A3(((s64)CV3(cv) << 12) + (MX31(mx) * VX(v))), 0);
			break;

		default:
			C2_MAC1 = A1(/*int44*/ (s64)((s64)CV1(cv) << 12) + (MX11(mx) * VX(v)) + (MX12(mx) * VY(v)) + (MX13(mx) * VZ(v)));
			C2_MAC2 = A2(/*int44*/ (s64)((s64)CV2(cv) << 12) + (MX21(mx) * VX(v)) + (MX22(mx) * VY(v)) + (MX23(mx) * VZ(v)));
			C2_MAC3 = A3(/*int44*/ (s64)((s64)CV3(cv) << 12) + (MX31(mx) * VX(v)) + (MX32(mx) * VY(v)) + (MX33(mx) * VZ(v)));
			break;
		}

		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		return 1;

	case 0x13:

		C2_MAC1 = A1((s64)(C2_L11 * C2_VX0) + (C2_L12 * C2_VY0) + (C2_L13 * C2_VZ0));
		C2_MAC2 = A2((s64)(C2_L21 * C2_VX0) + (C2_L22 * C2_VY0) + (C2_L23 * C2_VZ0));
		C2_MAC3 = A3((s64)(C2_L31 * C2_VX0) + (C2_L32 * C2_VY0) + (C2_L33 * C2_VZ0));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_MAC1 = A1(/*int44*/ (s64)((s64)C2_RBK << 12) + (C2_LR1 * C2_IR1) + (C2_LR2 * C2_IR2) + (C2_LR3 * C2_IR3));
		C2_MAC2 = A2(/*int44*/ (s64)((s64)C2_GBK << 12) + (C2_LG1 * C2_IR1) + (C2_LG2 * C2_IR2) + (C2_LG3 * C2_IR3));
		C2_MAC3 = A3(/*int44*/ (s64)((s64)C2_BBK << 12) + (C2_LB1 * C2_IR1) + (C2_LB2 * C2_IR2) + (C2_LB3 * C2_IR3));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_MAC1 = A1(((C2_R << 4) * C2_IR1) + (C2_IR0 * Lm_B1(A1(((s64)C2_RFC << 12) - ((C2_R << 4) * C2_IR1)), 0)));
		C2_MAC2 = A2(((C2_G << 4) * C2_IR2) + (C2_IR0 * Lm_B2(A2(((s64)C2_GFC << 12) - ((C2_G << 4) * C2_IR2)), 0)));
		C2_MAC3 = A3(((C2_B << 4) * C2_IR3) + (C2_IR0 * Lm_B3(A3(((s64)C2_BFC << 12) - ((C2_B << 4) * C2_IR3)), 0)));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_RGB0 = C2_RGB1;
		C2_RGB1 = C2_RGB2;
		C2_CD2 = C2_CODE;
		C2_R2 = Lm_C1(C2_MAC1 >> 4);
		C2_G2 = Lm_C2(C2_MAC2 >> 4);
		C2_B2 = Lm_C3(C2_MAC3 >> 4);
		return 1;

	case 0x14:

		C2_MAC1 = A1(/*int44*/ (s64)((s64)C2_RBK << 12) + (C2_LR1 * C2_IR1) + (C2_LR2 * C2_IR2) + (C2_LR3 * C2_IR3));
		C2_MAC2 = A2(/*int44*/ (s64)((s64)C2_GBK << 12) + (C2_LG1 * C2_IR1) + (C2_LG2 * C2_IR2) + (C2_LG3 * C2_IR3));
		C2_MAC3 = A3(/*int44*/ (s64)((s64)C2_BBK << 12) + (C2_LB1 * C2_IR1) + (C2_LB2 * C2_IR2) + (C2_LB3 * C2_IR3));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_MAC1 = A1(((C2_R << 4) * C2_IR1) + (C2_IR0 * Lm_B1(A1(((s64)C2_RFC << 12) - ((C2_R << 4) * C2_IR1)), 0)));
		C2_MAC2 = A2(((C2_G << 4) * C2_IR2) + (C2_IR0 * Lm_B2(A2(((s64)C2_GFC << 12) - ((C2_G << 4) * C2_IR2)), 0)));
		C2_MAC3 = A3(((C2_B << 4) * C2_IR3) + (C2_IR0 * Lm_B3(A3(((s64)C2_BFC << 12) - ((C2_B << 4) * C2_IR3)), 0)));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_RGB0 = C2_RGB1;
		C2_RGB1 = C2_RGB2;
		C2_CD2 = C2_CODE;
		C2_R2 = Lm_C1(C2_MAC1 >> 4);
		C2_G2 = Lm_C2(C2_MAC2 >> 4);
		C2_B2 = Lm_C3(C2_MAC3 >> 4);
		return 1;

	case 0x16:

		for (v = 0; v < 3; v++)
		{
			C2_MAC1 = A1((s64)(C2_L11 * VX(v)) + (C2_L12 * VY(v)) + (C2_L13 * VZ(v)));
			C2_MAC2 = A2((s64)(C2_L21 * VX(v)) + (C2_L22 * VY(v)) + (C2_L23 * VZ(v)));
			C2_MAC3 = A3((s64)(C2_L31 * VX(v)) + (C2_L32 * VY(v)) + (C2_L33 * VZ(v)));
			C2_IR1 = Lm_B1(C2_MAC1, lm);
			C2_IR2 = Lm_B2(C2_MAC2, lm);
			C2_IR3 = Lm_B3(C2_MAC3, lm);
			C2_MAC1 = A1(/*int44*/ (s64)((s64)C2_RBK << 12) + (C2_LR1 * C2_IR1) + (C2_LR2 * C2_IR2) + (C2_LR3 * C2_IR3));
			C2_MAC2 = A2(/*int44*/ (s64)((s64)C2_GBK << 12) + (C2_LG1 * C2_IR1) + (C2_LG2 * C2_IR2) + (C2_LG3 * C2_IR3));
			C2_MAC3 = A3(/*int44*/ (s64)((s64)C2_BBK << 12) + (C2_LB1 * C2_IR1) + (C2_LB2 * C2_IR2) + (C2_LB3 * C2_IR3));
			C2_IR1 = Lm_B1(C2_MAC1, lm);
			C2_IR2 = Lm_B2(C2_MAC2, lm);
			C2_IR3 = Lm_B3(C2_MAC3, lm);
			C2_MAC1 = A1(((C2_R << 4) * C2_IR1) + (C2_IR0 * Lm_B1(A1(((s64)C2_RFC << 12) - ((C2_R << 4) * C2_IR1)), 0)));
			C2_MAC2 = A2(((C2_G << 4) * C2_IR2) + (C2_IR0 * Lm_B2(A2(((s64)C2_GFC << 12) - ((C2_G << 4) * C2_IR2)), 0)));
			C2_MAC3 = A3(((C2_B << 4) * C2_IR3) + (C2_IR0 * Lm_B3(A3(((s64)C2_BFC << 12) - ((C2_B << 4) * C2_IR3)), 0)));
			C2_IR1 = Lm_B1(C2_MAC1, lm);
			C2_IR2 = Lm_B2(C2_MAC2, lm);
			C2_IR3 = Lm_B3(C2_MAC3, lm);
			C2_RGB0 = C2_RGB1;
			C2_RGB1 = C2_RGB2;
			C2_CD2 = C2_CODE;
			C2_R2 = Lm_C1(C2_MAC1 >> 4);
			C2_G2 = Lm_C2(C2_MAC2 >> 4);
			C2_B2 = Lm_C3(C2_MAC3 >> 4);
		}
		return 1;

	case 0x1b:

		C2_MAC1 = A1((s64)(C2_L11 * C2_VX0) + (C2_L12 * C2_VY0) + (C2_L13 * C2_VZ0));
		C2_MAC2 = A2((s64)(C2_L21 * C2_VX0) + (C2_L22 * C2_VY0) + (C2_L23 * C2_VZ0));
		C2_MAC3 = A3((s64)(C2_L31 * C2_VX0) + (C2_L32 * C2_VY0) + (C2_L33 * C2_VZ0));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_MAC1 = A1(/*int44*/ (s64)((s64)C2_RBK << 12) + (C2_LR1 * C2_IR1) + (C2_LR2 * C2_IR2) + (C2_LR3 * C2_IR3));
		C2_MAC2 = A2(/*int44*/ (s64)((s64)C2_GBK << 12) + (C2_LG1 * C2_IR1) + (C2_LG2 * C2_IR2) + (C2_LG3 * C2_IR3));
		C2_MAC3 = A3(/*int44*/ (s64)((s64)C2_BBK << 12) + (C2_LB1 * C2_IR1) + (C2_LB2 * C2_IR2) + (C2_LB3 * C2_IR3));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_MAC1 = A1((C2_R << 4) * C2_IR1);
		C2_MAC2 = A2((C2_G << 4) * C2_IR2);
		C2_MAC3 = A3((C2_B << 4) * C2_IR3);
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_RGB0 = C2_RGB1;
		C2_RGB1 = C2_RGB2;
		C2_CD2 = C2_CODE;
		C2_R2 = Lm_C1(C2_MAC1 >> 4);
		C2_G2 = Lm_C2(C2_MAC2 >> 4);
		C2_B2 = Lm_C3(C2_MAC3 >> 4);
		return 1;

	case 0x1c:

		C2_MAC1 = A1(/*int44*/ (s64)(((s64)C2_RBK) << 12) + (C2_LR1 * C2_IR1) + (C2_LR2 * C2_IR2) + (C2_LR3 * C2_IR3));
		C2_MAC2 = A2(/*int44*/ (s64)(((s64)C2_GBK) << 12) + (C2_LG1 * C2_IR1) + (C2_LG2 * C2_IR2) + (C2_LG3 * C2_IR3));
		C2_MAC3 = A3(/*int44*/ (s64)(((s64)C2_BBK) << 12) + (C2_LB1 * C2_IR1) + (C2_LB2 * C2_IR2) + (C2_LB3 * C2_IR3));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_MAC1 = A1((C2_R << 4) * C2_IR1);
		C2_MAC2 = A2((C2_G << 4) * C2_IR2);
		C2_MAC3 = A3((C2_B << 4) * C2_IR3);
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_RGB0 = C2_RGB1;
		C2_RGB1 = C2_RGB2;
		C2_CD2 = C2_CODE;
		C2_R2 = Lm_C1(C2_MAC1 >> 4);
		C2_G2 = Lm_C2(C2_MAC2 >> 4);
		C2_B2 = Lm_C3(C2_MAC3 >> 4);
		return 1;

	case 0x1e:

		C2_MAC1 = A1((s64)(C2_L11 * C2_VX0) + (C2_L12 * C2_VY0) + (C2_L13 * C2_VZ0));
		C2_MAC2 = A2((s64)(C2_L21 * C2_VX0) + (C2_L22 * C2_VY0) + (C2_L23 * C2_VZ0));
		C2_MAC3 = A3((s64)(C2_L31 * C2_VX0) + (C2_L32 * C2_VY0) + (C2_L33 * C2_VZ0));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_MAC1 = A1(/*int44*/ (s64)((s64)C2_RBK << 12) + (C2_LR1 * C2_IR1) + (C2_LR2 * C2_IR2) + (C2_LR3 * C2_IR3));
		C2_MAC2 = A2(/*int44*/ (s64)((s64)C2_GBK << 12) + (C2_LG1 * C2_IR1) + (C2_LG2 * C2_IR2) + (C2_LG3 * C2_IR3));
		C2_MAC3 = A3(/*int44*/ (s64)((s64)C2_BBK << 12) + (C2_LB1 * C2_IR1) + (C2_LB2 * C2_IR2) + (C2_LB3 * C2_IR3));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_RGB0 = C2_RGB1;
		C2_RGB1 = C2_RGB2;
		C2_CD2 = C2_CODE;
		C2_R2 = Lm_C1(C2_MAC1 >> 4);
		C2_G2 = Lm_C2(C2_MAC2 >> 4);
		C2_B2 = Lm_C3(C2_MAC3 >> 4);
		return 1;

	case 0x20:

		for (v = 0; v < 3; v++)
		{
			C2_MAC1 = A1((s64)(C2_L11 * VX(v)) + (C2_L12 * VY(v)) + (C2_L13 * VZ(v)));
			C2_MAC2 = A2((s64)(C2_L21 * VX(v)) + (C2_L22 * VY(v)) + (C2_L23 * VZ(v)));
			C2_MAC3 = A3((s64)(C2_L31 * VX(v)) + (C2_L32 * VY(v)) + (C2_L33 * VZ(v)));
			C2_IR1 = Lm_B1(C2_MAC1, lm);
			C2_IR2 = Lm_B2(C2_MAC2, lm);
			C2_IR3 = Lm_B3(C2_MAC3, lm);
			C2_MAC1 = A1(/*int44*/ (s64)((s64)C2_RBK << 12) + (C2_LR1 * C2_IR1) + (C2_LR2 * C2_IR2) + (C2_LR3 * C2_IR3));
			C2_MAC2 = A2(/*int44*/ (s64)((s64)C2_GBK << 12) + (C2_LG1 * C2_IR1) + (C2_LG2 * C2_IR2) + (C2_LG3 * C2_IR3));
			C2_MAC3 = A3(/*int44*/ (s64)((s64)C2_BBK << 12) + (C2_LB1 * C2_IR1) + (C2_LB2 * C2_IR2) + (C2_LB3 * C2_IR3));
			C2_IR1 = Lm_B1(C2_MAC1, lm);
			C2_IR2 = Lm_B2(C2_MAC2, lm);
			C2_IR3 = Lm_B3(C2_MAC3, lm);
			C2_RGB0 = C2_RGB1;
			C2_RGB1 = C2_RGB2;
			C2_CD2 = C2_CODE;
			C2_R2 = Lm_C1(C2_MAC1 >> 4);
			C2_G2 = Lm_C2(C2_MAC2 >> 4);
			C2_B2 = Lm_C3(C2_MAC3 >> 4);
		}
		return 1;

	case 0x28:

		C2_MAC1 = A1(C2_IR1 * C2_IR1);
		C2_MAC2 = A2(C2_IR2 * C2_IR2);
		C2_MAC3 = A3(C2_IR3 * C2_IR3);
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		return 1;

	case 0x29:

		C2_MAC1 = A1(((C2_R << 4) * C2_IR1) + (C2_IR0 * Lm_B1(A1(((s64)C2_RFC << 12) - ((C2_R << 4) * C2_IR1)), 0)));
		C2_MAC2 = A2(((C2_G << 4) * C2_IR2) + (C2_IR0 * Lm_B2(A2(((s64)C2_GFC << 12) - ((C2_G << 4) * C2_IR2)), 0)));
		C2_MAC3 = A3(((C2_B << 4) * C2_IR3) + (C2_IR0 * Lm_B3(A3(((s64)C2_BFC << 12) - ((C2_B << 4) * C2_IR3)), 0)));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_RGB0 = C2_RGB1;
		C2_RGB1 = C2_RGB2;
		C2_CD2 = C2_CODE;
		C2_R2 = Lm_C1(C2_MAC1 >> 4);
		C2_G2 = Lm_C2(C2_MAC2 >> 4);
		C2_B2 = Lm_C3(C2_MAC3 >> 4);
		return 1;

	case 0x2a:

		for (v = 0; v < 3; v++)
		{
			C2_MAC1 = A1((C2_R0 << 16) + (C2_IR0 * Lm_B1(A1(((s64)C2_RFC << 12) - (C2_R0 << 16)), 0)));
			C2_MAC2 = A2((C2_G0 << 16) + (C2_IR0 * Lm_B2(A2(((s64)C2_GFC << 12) - (C2_G0 << 16)), 0)));
			C2_MAC3 = A3((C2_B0 << 16) + (C2_IR0 * Lm_B3(A3(((s64)C2_BFC << 12) - (C2_B0 << 16)), 0)));
			C2_IR1 = Lm_B1(C2_MAC1, lm);
			C2_IR2 = Lm_B2(C2_MAC2, lm);
			C2_IR3 = Lm_B3(C2_MAC3, lm);
			C2_RGB0 = C2_RGB1;
			C2_RGB1 = C2_RGB2;
			C2_CD2 = C2_CODE;
			C2_R2 = Lm_C1(C2_MAC1 >> 4);
			C2_G2 = Lm_C2(C2_MAC2 >> 4);
			C2_B2 = Lm_C3(C2_MAC3 >> 4);
		}
		return 1;

	case 0x2d:

		C2_MAC0 = (int)(F((s64)(C2_ZSF3 * C2_SZ1) + (C2_ZSF3 * C2_SZ2) + (C2_ZSF3 * C2_SZ3)));
		C2_OTZ = Lm_D(m_mac0, 1);
		return 1;

	case 0x2e:

		C2_MAC0 = (int)(F((s64)(C2_ZSF4 * C2_SZ0) + (C2_ZSF4 * C2_SZ1) + (C2_ZSF4 * C2_SZ2) + (C2_ZSF4 * C2_SZ3)));
		C2_OTZ = Lm_D(m_mac0, 1);
		return 1;

	case 0x30:

		for (v = 0; v < 3; v++)
		{
			h_over_sz3 = GTE_RotTransPers(v, lm);
		}

		C2_MAC0 = (int)(F((s64)C2_DQB + ((s64)C2_DQA * h_over_sz3)));
		C2_IR0 = Lm_H(m_mac0, 1);
		return 1;

	case 0x3d:

		C2_MAC1 = A1(C2_IR0 * C2_IR1);
		C2_MAC2 = A2(C2_IR0 * C2_IR2);
		C2_MAC3 = A3(C2_IR0 * C2_IR3);
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_RGB0 = C2_RGB1;
		C2_RGB1 = C2_RGB2;
		C2_CD2 = C2_CODE;
		C2_R2 = Lm_C1(C2_MAC1 >> 4);
		C2_G2 = Lm_C2(C2_MAC2 >> 4);
		C2_B2 = Lm_C3(C2_MAC3 >> 4);
		return 1;

	case 0x3e:

		C2_MAC1 = A1(gte_shift(C2_MAC1, -m_sf) + (C2_IR0 * C2_IR1));
		C2_MAC2 = A2(gte_shift(C2_MAC2, -m_sf) + (C2_IR0 * C2_IR2));
		C2_MAC3 = A3(gte_shift(C2_MAC3, -m_sf) + (C2_IR0 * C2_IR3));
		C2_IR1 = Lm_B1(C2_MAC1, lm);
		C2_IR2 = Lm_B2(C2_MAC2, lm);
		C2_IR3 = Lm_B3(C2_MAC3, lm);
		C2_RGB0 = C2_RGB1;
		C2_RGB1 = C2_RGB2;
		C2_CD2 = C2_CODE;
		C2_R2 = Lm_C1(C2_MAC1 >> 4);
		C2_G2 = Lm_C2(C2_MAC2 >> 4);
		C2_B2 = Lm_C3(C2_MAC3 >> 4);
		return 1;

	case 0x3f:

		for (v = 0; v < 3; v++)
		{
			C2_MAC1 = A1((s64)(C2_L11 * VX(v)) + (C2_L12 * VY(v)) + (C2_L13 * VZ(v)));
			C2_MAC2 = A2((s64)(C2_L21 * VX(v)) + (C2_L22 * VY(v)) + (C2_L23 * VZ(v)));
			C2_MAC3 = A3((s64)(C2_L31 * VX(v)) + (C2_L32 * VY(v)) + (C2_L33 * VZ(v)));
			C2_IR1 = Lm_B1(C2_MAC1, lm);
			C2_IR2 = Lm_B2(C2_MAC2, lm);
			C2_IR3 = Lm_B3(C2_MAC3, lm);
			C2_MAC1 = A1(/*int44*/ (s64)((s64)C2_RBK << 12) + (C2_LR1 * C2_IR1) + (C2_LR2 * C2_IR2) + (C2_LR3 * C2_IR3));
			C2_MAC2 = A2(/*int44*/ (s64)((s64)C2_GBK << 12) + (C2_LG1 * C2_IR1) + (C2_LG2 * C2_IR2) + (C2_LG3 * C2_IR3));
			C2_MAC3 = A3(/*int44*/ (s64)((s64)C2_BBK << 12) + (C2_LB1 * C2_IR1) + (C2_LB2 * C2_IR2) + (C2_LB3 * C2_IR3));
			C2_IR1 = Lm_B1(C2_MAC1, lm);
			C2_IR2 = Lm_B2(C2_MAC2, lm);
			C2_IR3 = Lm_B3(C2_MAC3, lm);
			C2_MAC1 = A1((C2_R << 4) * C2_IR1);
			C2_MAC2 = A2((C2_G << 4) * C2_IR2);
			C2_MAC3 = A3((C2_B << 4) * C2_IR3);
			C2_IR1 = Lm_B1(C2_MAC1, lm);
			C2_IR2 = Lm_B2(C2_MAC2, lm);
			C2_IR3 = Lm_B3(C2_MAC3, lm);
			C2_RGB0 = C2_RGB1;
			C2_RGB1 = C2_RGB2;
			C2_CD2 = C2_CODE;
			C2_R2 = Lm_C1(C2_MAC1 >> 4);
			C2_G2 = Lm_C2(C2_MAC2 >> 4);
			C2_B2 = Lm_C3(C2_MAC3 >> 4);
		}
		return 1;
	}

	return 0;
}
