/*
 * Native forwarding derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/src/psx/LIBGTE.C
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#include <macros.h>
#include <psx/libgte.h>
#include <ctr_gte_transfer.h>

void SetTransMatrix(MATRIX *m)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native GTE storage can alias the input; retain read/write order.
	gte_SetTransMatrix(m);
#else
	// NOTE(aalhendi): SDK batches words through t0-t2; preferences are not an ABI.
	u32 x CTR_PSX_LOCAL_REGISTER_HINT("8") = (u32)m->t[0];
	u32 y CTR_PSX_LOCAL_REGISTER_HINT("9") = (u32)m->t[1];
	u32 z CTR_PSX_LOCAL_REGISTER_HINT("10") = (u32)m->t[2];
	CTR_GteWriteControl(x, 5);
	CTR_GteWriteControl(y, 6);
	CTR_GteWriteControl(z, 7);
#endif
}

void SetRotMatrix(MATRIX *m)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Preserve GTE normalization and aliased input ordering.
	gte_SetRotMatrix(m);
#else
	u32 rotationWord0 CTR_PSX_LOCAL_REGISTER_HINT("8") = CTR_ReadU32AlignedLE((const u8 *)m + 0);
	u32 rotationWord1 CTR_PSX_LOCAL_REGISTER_HINT("9") = CTR_ReadU32AlignedLE((const u8 *)m + 4);
	u32 rotationWord2 CTR_PSX_LOCAL_REGISTER_HINT("10") = CTR_ReadU32AlignedLE((const u8 *)m + 8);
	u32 rotationWord3 CTR_PSX_LOCAL_REGISTER_HINT("11") = CTR_ReadU32AlignedLE((const u8 *)m + 12);
	u32 rotationWord4 CTR_PSX_LOCAL_REGISTER_HINT("12") = CTR_ReadU32AlignedLE((const u8 *)m + 16);
	CTR_GteWriteControl(rotationWord0, 0);
	CTR_GteWriteControl(rotationWord1, 1);
	CTR_GteWriteControl(rotationWord2, 2);
	CTR_GteWriteControl(rotationWord3, 3);
	CTR_GteWriteControl(rotationWord4, 4);
#endif
}

void SetGeomOffset(s32 ofx, s32 ofy)
{
	u32 offsetX = (u32)ofx << 16;
	u32 offsetY = (u32)ofy << 16;
	CTR_GteWriteControl(offsetX, 24);
	CTR_GteWriteControl(offsetY, 25);
}

void SetLightMatrix(MATRIX *m)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Preserve GTE normalization and aliased input ordering.
	gte_SetLightMatrix(m);
#else
	// NOTE(aalhendi): SDK preloads t0-t4; these measured preferences are not an ABI.
	u32 lightWord0 CTR_PSX_LOCAL_REGISTER_HINT("8") = CTR_ReadU32AlignedLE((const u8 *)m + 0);
	u32 lightWord1 CTR_PSX_LOCAL_REGISTER_HINT("9") = CTR_ReadU32AlignedLE((const u8 *)m + 4);
	u32 lightWord2 CTR_PSX_LOCAL_REGISTER_HINT("10") = CTR_ReadU32AlignedLE((const u8 *)m + 8);
	u32 lightWord3 CTR_PSX_LOCAL_REGISTER_HINT("11") = CTR_ReadU32AlignedLE((const u8 *)m + 12);
	u32 lightWord4 CTR_PSX_LOCAL_REGISTER_HINT("12") = CTR_ReadU32AlignedLE((const u8 *)m + 16);
	CTR_GteWriteControl(lightWord0, 8);
	CTR_GteWriteControl(lightWord1, 9);
	CTR_GteWriteControl(lightWord2, 10);
	CTR_GteWriteControl(lightWord3, 11);
	CTR_GteWriteControl(lightWord4, 12);
#endif
}

void RotTrans(SVECTOR *v0, VECTOR *v1, s32 *flag)
{
#ifdef CTR_NATIVE
	// NOTE(aalhendi): Native reads typed XYZ and clears inactive input padding.
	CTR_GteLoadSV0(v0);
	gte_rt();
	gte_stlvnl(v1);
	gte_stflg(flag);
#else
	// NOTE(aalhendi): The SDK vector owns both LWC2 words; this cast is only an assembly operand.
	CTR_GteLoadSVec3V0((const SVec3 *)v0);
	CTR_GteRegisterReadDelay();
	gte_rt();
	CTR_PSX_STORE_COP2_VEC3(&v1->vx, 25, 26, 27);
	{
		s32 value = CFC2_S(31);
		CTR_WriteU32AlignedLE(flag, (u32)value);
	}
#endif
}
