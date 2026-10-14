/*
 * Derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/src/psx/INLINE_C.C
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#include <macros.h>
#include <platform/native_pgxp.h>
#include <psx/gtereg.h>
#include <psx/inline_c.h>
#include <psx/libgte.h>

extern u32 gte_leadingzerocount(u32 lzcs);

// MFC2, MFC2_S, MTC2 and MTC2_S each need one PGXP hook, and the hook differs
// only by direction, so it lives here rather than being repeated four times.
static void NativeGte_PgxpOnRead(s32 reg)
{
	if (((reg >= 12) && (reg <= 15)) && NATIVE_PGXP_ACTIVE())
	{
		// Culling helpers re-read the register they just projected; remember the
		// vertex so a later MTC2 of the same value can recover it.
		NativePgxp_GteReadSXY(reg, gteRegs.CP2D.p[reg].d);
	}
}

static void NativeGte_PgxpOnWrite(u32 value, s32 reg)
{
	if (!NATIVE_PGXP_ACTIVE())
	{
		return;
	}

	// A write is what drops the register's exact value, so that is where it has
	// to be retired. Invalidating on the read instead would clear an exact input
	// that a projection is about to consume, and would leave a stale one in place
	// after the overwrite it was meant to guard against.
	if (reg <= 5)
	{
		NativePgxp_GteInvalidateInput(reg);
	}
	else if ((reg >= 12) && (reg <= 15))
	{
		// Game code saves and restores SXY registers around helper calls, or
		// reloads them from a stored vertex to continue a strip.
		NativePgxp_GteWriteSXY(reg, value);
	}
}

u32 MFC2(s32 reg)
{
	switch (reg)
	{
	case 1:
	case 3:
	case 5:
	case 8:
	case 9:
	case 10:
	case 11:
		gteRegs.CP2D.p[reg].d = (s32)gteRegs.CP2D.p[reg].sw.l;
		break;

	case 7:
	case 16:
	case 17:
	case 18:
	case 19:
		gteRegs.CP2D.p[reg].d = (u32)gteRegs.CP2D.p[reg].w.l;
		break;

	case 15:
		gteRegs.CP2D.p[reg].d = C2_SXY2;
		break;

	case 28:
	case 29:
		gteRegs.CP2D.p[reg].d = LIM(C2_IR1 >> 7, 0x1f, 0, 0) | (LIM(C2_IR2 >> 7, 0x1f, 0, 0) << 5) | (LIM(C2_IR3 >> 7, 0x1f, 0, 0) << 10);
		break;
	}

	NativeGte_PgxpOnRead(reg);

	return gteRegs.CP2D.p[reg].d;
}

s32 MFC2_S(s32 reg)
{
	// TODO(aalhendi): Confirm whether modifier reads should also use signed semantics.
	switch (reg)
	{
	case 1:
	case 3:
	case 5:
	case 8:
	case 9:
	case 10:
	case 11:
		gteRegs.CP2D.p[reg].d = (s32)gteRegs.CP2D.p[reg].sw.l;
		break;

	case 7:
	case 16:
	case 17:
	case 18:
	case 19:
		gteRegs.CP2D.p[reg].d = (u32)gteRegs.CP2D.p[reg].w.l;
		break;

	case 15:
		gteRegs.CP2D.p[reg].d = C2_SXY2;
		break;

	case 28:
	case 29:
		gteRegs.CP2D.p[reg].d = LIM(C2_IR1 >> 7, 0x1f, 0, 0) | (LIM(C2_IR2 >> 7, 0x1f, 0, 0) << 5) | (LIM(C2_IR3 >> 7, 0x1f, 0, 0) << 10);
		break;
	}

	NativeGte_PgxpOnRead(reg);

	return gteRegs.CP2D.p[reg].sd;
}

void MTC2(u32 value, s32 reg)
{
	NativeGte_PgxpOnWrite(value, reg);

	switch (reg)
	{
	case 15:
		C2_SXY0 = C2_SXY1;
		C2_SXY1 = C2_SXY2;
		C2_SXY2 = value;
		break;

	case 28:
		C2_IR1 = (value & 0x1f) << 7;
		C2_IR2 = (value & 0x3e0) << 2;
		C2_IR3 = (value & 0x7c00) >> 3;
		break;

	case 30:
		C2_LZCR = gte_leadingzerocount(value);
		break;

	case 31:
		return;
	}

	gteRegs.CP2D.p[reg].d = value;
}

void MTC2_S(s32 value, s32 reg)
{
	NativeGte_PgxpOnWrite((u32)value, reg);

	switch (reg)
	{
	case 15:
		C2_SXY0 = C2_SXY1;
		C2_SXY1 = C2_SXY2;
		C2_SXY2 = value;
		break;

	case 28:
		C2_IR1 = (value & 0x1f) << 7;
		C2_IR2 = (value & 0x3e0) << 2;
		C2_IR3 = (value & 0x7c00) >> 3;
		break;

	case 30:
		C2_LZCR = gte_leadingzerocount(value);
		break;

	case 31:
		return;
	}

	gteRegs.CP2D.p[reg].sd = value;
}

void CTC2(u32 value, s32 reg)
{
	// A control-register write can land in any matrix bank, so the transform
	// shadows it might have overwritten are retired here.
	NativePgxp_InvalidateTransform(reg);

	switch (reg)
	{
	case 4:
	case 12:
	case 20:
	case 26:
	case 27:
	case 29:
	case 30:
		value = (u32)(s32)(s16)value;
		break;

	case 31:
		value = value & 0x7ffff000;
		if ((value & 0x7f87e000) != 0)
		{
			value |= 0x80000000;
		}
		break;
	}

	gteRegs.CP2C.p[reg].d = value;
}

void CTC2_S(s32 value, s32 reg)
{
	NativePgxp_InvalidateTransform(reg);

	switch (reg)
	{
	case 4:
	case 12:
	case 20:
	case 26:
	case 27:
	case 29:
	case 30:
		value = (s32)(s16)value;
		break;

	case 31:
		value = value & 0x7ffff000;
		if ((value & 0x7f87e000) != 0)
		{
			value |= 0x80000000;
		}
		break;
	}

	gteRegs.CP2C.p[reg].sd = value;
}

u32 CFC2(s32 reg)
{
	// TODO(aalhendi): Verify CFC2 control-register edge cases against PS1 GTE behavior.

	return gteRegs.CP2C.p[reg].d;
}

s32 CFC2_S(s32 reg)
{
	// TODO(aalhendi): Verify CFC2 control-register edge cases against PS1 GTE behavior.

	return gteRegs.CP2C.p[reg].sd;
}

s32 doCOP2(s32 op)
{
	return GTE_operator(op);
}
