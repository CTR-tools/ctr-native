/*
 * Derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/src/psx/LIBGTE.C
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#include <macros.h>
#include <ctr_gte.h>
#include <psx/gtemac.h>
#include <psx/gtereg.h>
#include <psx/inline_c.h>
#include <psx/libgte.h>

#include <assert.h>
#include <stdio.h>

#include "native_gte_rcossin_tbl.h"
#include "native_gte_ratan_tbl.h"
#include "native_gte_sqrt_tbl.h"

extern u32 gte_leadingzerocount(u32 lzcs);

#define ONE_BITS 12
#define ONE      (1 << 12)
#define FIXED(a) ((a) >> 12)

#ifndef MIN
#define MIN(a, b) fst_min(a, b)
#endif

#ifndef MAX
#define MAX(a, b) fst_max(a, b)
#endif

internal inline s32 fst_min(s32 a, s32 b)
{
	s32 diff = a - b;
	s32 dsgn = diff >> 31;
	return b + (diff & dsgn);
}

internal inline s32 fst_max(s32 a, s32 b)
{
	s32 diff = a - b;
	s32 dsgn = diff >> 31;
	return a - (diff & dsgn);
}

void InitGeom()
{
	C2_ZSF3 = 341;
	C2_ZSF4 = 256;
	C2_H = 1000;
	C2_DQA = -98;
	C2_DQB = 340;
	C2_OFX = 0;
	C2_OFY = 0;
}

void SetGeomOffset(int ofx, int ofy)
{
	C2_OFX = (ofx << 16);
	C2_OFY = (ofy << 16);
}

void SetGeomScreen(int h)
{
	C2_H = h;
}

void SetRotMatrix(MATRIX *m)
{
	gte_SetRotMatrix(m);
}

void SetLightMatrix(MATRIX *m)
{
	gte_SetLightMatrix(m);
}

void SetColorMatrix(MATRIX *m)
{
	gte_SetColorMatrix(m);
}

void SetTransMatrix(MATRIX *m)
{
	gte_SetTransMatrix(m);
}

#define MAX_NUM_MATRICES 20
s32 matrixLevel = 0;
MATRIX stack[MAX_NUM_MATRICES];    // unk_410
MATRIX *currentMatrix = &stack[0]; // unk_40C

void PushMatrix()
{
	if (matrixLevel < 20)
	{
		MATRIX *m = &stack[matrixLevel]; //$t7

		gte_ReadRotMatrix(m);
		gte_sttr(m->t);

		currentMatrix++;
		matrixLevel++;
	}
	else
	{
		printf("Error: Can't push matrix,stack(max 20) is full!\n");
	}
}

void PopMatrix()
{
	if (matrixLevel > 0)
	{
		currentMatrix--;
		matrixLevel--;
		MATRIX *m = &stack[matrixLevel]; //$t7

		gte_SetRotMatrix(m);
		gte_SetTransMatrix(m);
	}
	else
	{
		printf("Error: Can't pop matrix,stack is empty!\n");
	}
}

void RotTrans(SVECTOR *v0, VECTOR *v1, s32 *flag)
{
	gte_RotTrans(v0, v1, flag);
}

void RotTransSV(SVECTOR *v0, SVECTOR *v1, s32 *flag)
{
	CTR_GteLoadSV0(v0);
	gte_rt();
	CTR_GteStoreSV(v1);
	gte_stflg(flag);
}

int RotTransPers(SVECTOR *v0, s32 *sxy, s32 *p, s32 *flag)
{
	int sz;
	gte_RotTransPers(v0, sxy, p, flag, &sz);

	return sz;
}

int RotTransPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, s32 *sxy0, s32 *sxy1, s32 *sxy2, s32 *p, s32 *flag)
{
	int sz;
	gte_RotTransPers3(v0, v1, v2, sxy0, sxy1, sxy2, p, flag, &sz);

	return sz;
}

int RotTransPers4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, s32 *sxy0, s32 *sxy1, s32 *sxy2, s32 *sxy3, s32 *p, s32 *flag)
{
	int _flag;
	int sz;

	CTR_GteLoadSV3(v0, v1, v2);
	gte_rtpt();

	CTR_GteStoreSXY3(sxy0, sxy1, sxy2);

	gte_stflg(&_flag);

	CTR_GteLoadSV0(v3);
	gte_rtps();

	CTR_GteStoreSXY(sxy3);
	gte_stflg(flag);
	gte_stdp(p);

	*flag |= _flag;
	gte_stszotz(&sz);

	return sz;
}

void NormalColor(SVECTOR *v0, CVECTOR *v1)
{
	gte_NormalColor(v0, v1);
}

void NormalColor3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4, CVECTOR *v5)
{
	gte_NormalColor3(v0, v1, v2, v3, v4, v5);
}

void NormalColorDpq(SVECTOR *v0, CVECTOR *v1, int p, CVECTOR *v2)
{
	gte_NormalColorDpq(v0, v1, p, v2);
}

void NormalColorCol(SVECTOR *v0, CVECTOR *v1, CVECTOR *v2)
{
	gte_NormalColorCol(v0, v1, v2);
}

void NormalColorCol3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, CVECTOR *v3, CVECTOR *v4, CVECTOR *v5, CVECTOR *v6)
{
	gte_NormalColorCol3(v0, v1, v2, v3, v4, v5, v6);
}

void DpqColor(CVECTOR *v0, int p, CVECTOR *v1)
{
	gte_DpqColor(v0, &p, v1);
}

void ColorDpq(VECTOR *v0, CVECTOR *v1, int p, CVECTOR *v2)
{
	gte_ColorDpq(v0, v1, p, v2);
}

void ColorCol(VECTOR *v0, CVECTOR *v1, CVECTOR *v2)
{
	gte_ColorCol(v0, v1, v2);
}

int NormalClip(int sxy0, int sxy1, int sxy2)
{
	int opz;

	gte_NormalClip(&sxy0, &sxy1, &sxy2, &opz);

	return opz;
}

void LocalLight(SVECTOR *v0, VECTOR *v1)
{
	gte_LocalLight(v0, v1);
}

int RotAverageNclip4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3, s32 *sxy0, s32 *sxy1, s32 *sxy2, s32 *sxy3, s32 *p, s32 *otz, s32 *flag)
{
	CTR_GteLoadSV3(v0, v1, v2);
	gte_rtpt();
	gte_stflg(flag);

	gte_nclip();

	int opz;
	gte_stopz(&opz);

	if (opz > 0)
	{
		CTR_GteStoreSXY3(sxy0, sxy1, sxy2);

		CTR_GteLoadSV0(v3);

		gte_rtps();

		CTR_GteStoreSXY(sxy3);
		gte_stdp(p);
		gte_stflg(flag);

		gte_avsz4();

		gte_stotz(otz);
	}

	return opz;
}

MATRIX *MulMatrix0(MATRIX *m0, MATRIX *m1, MATRIX *m2)
{
	// correct Psy-Q implementation
	SVECTOR v0, r0, r1, r2;

	gte_SetRotMatrix(m0);

	v0.vx = m1->m[0][0];
	v0.vy = m1->m[1][0];
	v0.vz = m1->m[2][0];

	CTR_GteLoadSV0(&v0);
	gte_rtv0();
	CTR_GteStoreSV(&r0);

	v0.vx = m1->m[0][1];
	v0.vy = m1->m[1][1];
	v0.vz = m1->m[2][1];

	CTR_GteLoadSV0(&v0);
	gte_rtv0();
	CTR_GteStoreSV(&r1);

	v0.vx = m1->m[0][2];
	v0.vy = m1->m[1][2];
	v0.vz = m1->m[2][2];

	CTR_GteLoadSV0(&v0);
	gte_rtv0();
	CTR_GteStoreSV(&r2);

	m2->m[0][0] = r0.vx;
	m2->m[0][1] = r1.vx;
	m2->m[0][2] = r2.vx;

	m2->m[1][0] = r0.vy;
	m2->m[1][1] = r1.vy;
	m2->m[1][2] = r2.vy;

	m2->m[2][0] = r0.vz;
	m2->m[2][1] = r1.vz;
	m2->m[2][2] = r2.vz;


	return m2;
}

MATRIX *MulMatrix(MATRIX *m0, MATRIX *m1)
{
	MATRIX tmp;
	gte_MulMatrix0(m0, m1, &tmp);

	*m0 = tmp;

	return m0;
}

MATRIX *MulMatrix2(MATRIX *m0, MATRIX *m1)
{
	// Same as MulMatrix but result goes to m1
	MATRIX tmp;
	gte_MulMatrix0(m0, m1, &tmp);

	*m1 = tmp;

	return m1;
}

MATRIX *MulRotMatrix(MATRIX *m0)
{
	// TODO(aalhendi): Verify against PsyQ behavior; RTV0 may be insufficient.
	CTR_GteLoadS16TripletV0(&m0->m[0][0]);
	gte_rtv0();
	CTR_GteStoreS16Triplet(&m0->m[0][0]);

	CTR_GteLoadS16TripletV0(&m0->m[1][0]);
	gte_rtv0();
	CTR_GteStoreS16Triplet(&m0->m[1][0]);

	CTR_GteLoadS16TripletV0(&m0->m[2][0]);
	gte_rtv0();
	CTR_GteStoreS16Triplet(&m0->m[2][0]);

	return m0;
}

void SetBackColor(int rbk, int gbk, int bbk)
{
	gte_SetBackColor(rbk, gbk, bbk);
}

void SetFarColor(int rfc, int gfc, int bfc)
{
	gte_SetFarColor(rfc, gfc, bfc);
}

VECTOR *ApplyMatrix(MATRIX *m, SVECTOR *v0, VECTOR *v1)
{
	gte_SetRotMatrix(m);
	CTR_GteLoadSV0(v0);
	gte_rtv0();
	CTR_GteStoreMAC(&v1->vx);
	return v1;
}

VECTOR *ApplyRotMatrix(SVECTOR *v0, VECTOR *v1)
{
	CTR_GteLoadSV0(v0);
	gte_rtv0();
	CTR_GteStoreMAC(&v1->vx);
	return v1;
}

VECTOR *ApplyRotMatrixLV(VECTOR *v0, VECTOR *v1)
{
	VECTOR tmpHI;
	VECTOR tmpLO;

	tmpHI.vx = v0->vx;
	tmpHI.vy = v0->vy;
	tmpHI.vz = v0->vz;

	if (tmpHI.vx < 0)
	{
		tmpLO.vx = -(-tmpHI.vx >> 0xf);
		tmpHI.vx = -(-tmpHI.vx & 0x7fff);
	}
	else
	{
		tmpLO.vx = tmpHI.vx >> 0xf;
		tmpHI.vx = tmpHI.vx & 0x7fff;
	}

	if (tmpHI.vy < 0)
	{
		tmpLO.vy = -(-tmpHI.vy >> 0xf);
		tmpHI.vy = -(-tmpHI.vy & 0x7fff);
	}
	else
	{
		tmpLO.vy = tmpHI.vy >> 0xf;
		tmpHI.vy = tmpHI.vy & 0x7fff;
	}

	if (tmpHI.vz < 0)
	{
		tmpLO.vz = -(-tmpHI.vz >> 0xf);
		tmpHI.vz = -(-tmpHI.vz & 0x7fff);
	}
	else
	{
		tmpLO.vz = tmpHI.vz >> 0xf;
		tmpHI.vz = tmpHI.vz & 0x7fff;
	}

	CTR_GteLoadLVL(&tmpLO.vx);
	gte_rtir_sf0();
	CTR_GteStoreMAC(&tmpLO.vx);

	CTR_GteLoadLVL(&tmpHI.vx);
	gte_rtir();

	if (tmpLO.vx < 0)
	{
		tmpLO.vx *= 8;
	}
	else
	{
		tmpLO.vx <<= 3;
	}

	if (tmpLO.vy < 0)
	{
		tmpLO.vy *= 8;
	}
	else
	{
		tmpLO.vy <<= 3;
	}

	if (tmpLO.vz < 0)
	{
		tmpLO.vz *= 8;
	}
	else
	{
		tmpLO.vz <<= 3;
	}

	CTR_GteStoreMAC(&tmpHI.vx);

	v1->vx = tmpHI.vx + tmpLO.vx;
	v1->vy = tmpHI.vy + tmpLO.vy;
	v1->vz = tmpHI.vz + tmpLO.vz;
	return v1;
}

SVECTOR *ApplyMatrixSV(MATRIX *m, SVECTOR *v0, SVECTOR *v1)
{
	gte_SetRotMatrix(m);
	CTR_GteLoadSV0(v0);
	gte_rtv0();
	CTR_GteStoreSV(v1);
	return v1;
}

VECTOR *ApplyMatrixLV(MATRIX *m, VECTOR *v0, VECTOR *v1)
{
	VECTOR tmpHI;
	VECTOR tmpLO;

	gte_SetRotMatrix(m);

	tmpHI.vx = v0->vx;
	tmpHI.vy = v0->vy;
	tmpHI.vz = v0->vz;

	if (tmpHI.vx < 0)
	{
		tmpLO.vx = -(-tmpHI.vx >> 0xf);
		tmpHI.vx = -(-tmpHI.vx & 0x7fff);
	}
	else
	{
		tmpLO.vx = tmpHI.vx >> 0xf;
		tmpHI.vx = tmpHI.vx & 0x7fff;
	}

	if (tmpHI.vy < 0)
	{
		tmpLO.vy = -(-tmpHI.vy >> 0xf);
		tmpHI.vy = -(-tmpHI.vy & 0x7fff);
	}
	else
	{
		tmpLO.vy = tmpHI.vy >> 0xf;
		tmpHI.vy = tmpHI.vy & 0x7fff;
	}

	if (tmpHI.vz < 0)
	{
		tmpLO.vz = -(-tmpHI.vz >> 0xf);
		tmpHI.vz = -(-tmpHI.vz & 0x7fff);
	}
	else
	{
		tmpLO.vz = tmpHI.vz >> 0xf;
		tmpHI.vz = tmpHI.vz & 0x7fff;
	}

	CTR_GteLoadLVL(&tmpLO.vx);
	gte_rtir_sf0();
	CTR_GteStoreMAC(&tmpLO.vx);

	CTR_GteLoadLVL(&tmpHI.vx);
	gte_rtir();

	if (tmpLO.vx < 0)
	{
		tmpLO.vx *= 8;
	}
	else
	{
		tmpLO.vx <<= 3;
	}

	if (tmpLO.vy < 0)
	{
		tmpLO.vy *= 8;
	}
	else
	{
		tmpLO.vy <<= 3;
	}

	if (tmpLO.vz < 0)
	{
		tmpLO.vz *= 8;
	}
	else
	{
		tmpLO.vz <<= 3;
	}

	CTR_GteStoreMAC(&tmpHI.vx);

	v1->vx = tmpHI.vx + tmpLO.vx;
	v1->vy = tmpHI.vy + tmpLO.vy;
	v1->vz = tmpHI.vz + tmpLO.vz;
	return v1;
}

MATRIX *RotMatrix(SVECTOR *r, MATRIX *m)
{
	// correct Psy-Q implementation
	int c0, c1, c2;
	int s0, s1, s2;
	int s2p0, s2m0, c2p0, c2m0;
	int s2c0, s2s0, c2c0, c2s0;

	c0 = rcos(r->vx);
	c1 = rcos(r->vy);
	c2 = rcos(r->vz);
	s0 = rsin(r->vx);
	s1 = rsin(r->vy);
	s2 = rsin(r->vz);
	s2p0 = rsin(r->vz + r->vx);
	s2m0 = rsin(r->vz - r->vx);
	c2p0 = rcos(r->vz + r->vx);
	c2m0 = rcos(r->vz - r->vx);
	s2c0 = (s2p0 + s2m0) / 2;
	c2s0 = (s2p0 - s2m0) / 2;
	s2s0 = (c2m0 - c2p0) / 2;
	c2c0 = (c2m0 + c2p0) / 2;

	m->m[0][0] = FIXED(c2 * c1);
	m->m[1][0] = s2c0 + FIXED(c2s0 * s1);
	m->m[2][0] = s2s0 - FIXED(c2c0 * s1);
	m->m[0][1] = -FIXED(s2 * c1);
	m->m[1][1] = c2c0 - FIXED(s2s0 * s1);
	m->m[2][1] = c2s0 + FIXED(s2c0 * s1);
	m->m[0][2] = s1;
	m->m[1][2] = -FIXED(c1 * s0);
	m->m[2][2] = FIXED(c1 * c0);

	return m;
}

MATRIX *RotMatrixYXZ(SVECTOR *r, MATRIX *m)
{
	// correct Psy-Q implementation
	int c0, c1, c2;
	int s0, s1, s2;

	c0 = rcos(r->vx);
	c1 = rcos(r->vy);
	c2 = rcos(r->vz);
	s0 = rsin(r->vx);
	s1 = rsin(r->vy);
	s2 = rsin(r->vz);

	// Y-axis
	m->m[1][0] = FIXED(s2 * c0);
	m->m[1][1] = FIXED(c2 * c0);
	m->m[1][2] = -s0;

	// X-axis
	int x0 = FIXED(s1 * s0);
	m->m[0][0] = FIXED(c1 * c2) + FIXED(x0 * s2);
	m->m[0][1] = FIXED(x0 * c2) - FIXED(c1 * s2);
	m->m[0][2] = FIXED(s1 * c0);

	// Z-axis
	int z0 = FIXED(c1 * s0);
	m->m[2][1] = FIXED(s1 * s2) + FIXED(z0 * c2);
	m->m[2][0] = FIXED(z0 * s2) - FIXED(s1 * c2);
	m->m[2][2] = FIXED(c1 * c0);

	return m;
}

MATRIX *RotMatrixX(int r, MATRIX *m)
{
	// correct Psy-Q implementation
	int s0 = rsin(r);
	int c0 = rcos(r);
	int t1, t2;
	t1 = m->m[1][0];
	t2 = m->m[2][0];
	m->m[1][0] = FIXED(t1 * c0 - t2 * s0);
	m->m[2][0] = FIXED(t1 * s0 + t2 * c0);
	t1 = m->m[1][1];
	t2 = m->m[2][1];
	m->m[1][1] = FIXED(t1 * c0 - t2 * s0);
	m->m[2][1] = FIXED(t1 * s0 + t2 * c0);
	t1 = m->m[1][2];
	t2 = m->m[2][2];
	m->m[1][2] = FIXED(t1 * c0 - t2 * s0);
	m->m[2][2] = FIXED(t1 * s0 + t2 * c0);

	return m;
}

MATRIX *RotMatrixY(int r, MATRIX *m)
{
	// correct Psy-Q implementation
	int s0 = rsin(r);
	int c0 = rcos(r);
	int t1, t2;
	t1 = m->m[0][0];
	t2 = m->m[2][0];
	m->m[0][0] = FIXED(t1 * c0 + t2 * s0);
	m->m[2][0] = FIXED(-t1 * s0 + t2 * c0);
	t1 = m->m[0][1];
	t2 = m->m[2][1];
	m->m[0][1] = FIXED(t1 * c0 + t2 * s0);
	m->m[2][1] = FIXED(-t1 * s0 + t2 * c0);
	t1 = m->m[0][2];
	t2 = m->m[2][2];
	m->m[0][2] = FIXED(t1 * c0 + t2 * s0);
	m->m[2][2] = FIXED(-t1 * s0 + t2 * c0);

	return m;
}

MATRIX *RotMatrixZ(int r, MATRIX *m)
{
	// correct Psy-Q implementation
	int s0 = rsin(r);
	int c0 = rcos(r);
	int t1, t2;
	t1 = m->m[0][0];
	t2 = m->m[1][0];
	m->m[0][0] = FIXED(t1 * c0 - t2 * s0);
	m->m[1][0] = FIXED(t1 * s0 + t2 * c0);
	t1 = m->m[0][1];
	t2 = m->m[1][1];
	m->m[0][1] = FIXED(t1 * c0 - t2 * s0);
	m->m[1][1] = FIXED(t1 * s0 + t2 * c0);
	t1 = m->m[0][2];
	t2 = m->m[1][2];
	m->m[0][2] = FIXED(t1 * c0 - t2 * s0);
	m->m[1][2] = FIXED(t1 * s0 + t2 * c0);

	return m;
}

MATRIX *RotMatrixZYX_gte(SVECTOR *r, MATRIX *m)
{
	m->m[0][0] = 0x1000;
	m->m[0][1] = 0;
	m->m[0][2] = 0;

	m->m[1][0] = 0;
	m->m[1][1] = 0x1000;
	m->m[1][2] = 0;

	m->m[2][0] = 0;
	m->m[2][1] = 0;
	m->m[2][2] = 0x1000;

	RotMatrixX(r->vx, m);
	RotMatrixY(r->vy, m);
	RotMatrixZ(r->vz, m);
	return m;
}

MATRIX *CompMatrix(MATRIX *m0, MATRIX *m1, MATRIX *m2)
{
	// TODO(aalhendi): Validate this against PsyQ behavior if CTR hits it.
	SVECTOR tmp;
	gte_MulMatrix0(m0, m1, m2);

	tmp.vx = m1->t[0];
	tmp.vy = m1->t[1];
	tmp.vz = m1->t[2];

	CTR_GteLoadSV0(&tmp);
	gte_rtv0();
	CTR_GteStoreMAC(m2->t);

	m2->t[0] += m0->t[0];
	m2->t[1] += m0->t[1];
	m2->t[2] += m0->t[2];

	return m2;
}

MATRIX *CompMatrixLV(MATRIX *m0, MATRIX *m1, MATRIX *m2)
{
	// TODO(aalhendi): Validate this against PsyQ behavior if CTR hits it.
	VECTOR tmpHI;
	VECTOR tmpLO;

	gte_MulMatrix0(m0, m1, m2);

	// next... same as ApplyMatrixLV
	tmpHI.vx = m1->t[0];
	tmpHI.vy = m1->t[1];
	tmpHI.vz = m1->t[2];

	if (tmpHI.vx < 0)
	{
		tmpLO.vx = -(-tmpHI.vx >> 0xf);
		tmpHI.vx = -(-tmpHI.vx & 0x7fff);
	}
	else
	{
		tmpLO.vx = tmpHI.vx >> 0xf;
		tmpHI.vx = tmpHI.vx & 0x7fff;
	}

	if (tmpHI.vy < 0)
	{
		tmpLO.vy = -(-tmpHI.vy >> 0xf);
		tmpHI.vy = -(-tmpHI.vy & 0x7fff);
	}
	else
	{
		tmpLO.vy = tmpHI.vy >> 0xf;
		tmpHI.vy = tmpHI.vy & 0x7fff;
	}

	if (tmpHI.vz < 0)
	{
		tmpLO.vz = -(-tmpHI.vz >> 0xf);
		tmpHI.vz = -(-tmpHI.vz & 0x7fff);
	}
	else
	{
		tmpLO.vz = tmpHI.vz >> 0xf;
		tmpHI.vz = tmpHI.vz & 0x7fff;
	}

	CTR_GteLoadLVL(&tmpLO.vx);
	gte_rtir_sf0();
	CTR_GteStoreMAC(&tmpLO.vx);

	CTR_GteLoadLVL(&tmpHI.vx);
	gte_rtir();

	if (tmpLO.vx < 0)
	{
		tmpLO.vx = tmpLO.vx * 8;
	}
	else
	{
		tmpLO.vx = tmpLO.vx << 3;
	}

	if (tmpLO.vy < 0)
	{
		tmpLO.vy = tmpLO.vy * 8;
	}
	else
	{
		tmpLO.vy = tmpLO.vy << 3;
	}

	if (tmpLO.vz < 0)
	{
		tmpLO.vz = tmpLO.vz * 8;
	}
	else
	{
		tmpLO.vz = tmpLO.vz << 3;
	}

	CTR_GteStoreMAC(&tmpHI.vx);

	m2->t[0] = tmpHI.vx + tmpLO.vx + m0->t[0];
	m2->t[1] = tmpHI.vy + tmpLO.vy + m0->t[1];
	m2->t[2] = tmpHI.vz + tmpLO.vz + m0->t[2];

	return m2;
}

MATRIX *TransMatrix(MATRIX *m, VECTOR *v)
{
	m->t[0] = v->vx;
	m->t[1] = v->vy;
	m->t[2] = v->vz;
	return m;
}

MATRIX *ScaleMatrix(MATRIX *m, VECTOR *v)
{
	m->m[0][0] = FIXED(m->m[0][0] * v->vx);
	m->m[0][1] = FIXED(m->m[0][1] * v->vx);
	m->m[0][2] = FIXED(m->m[0][2] * v->vx);
	m->m[1][0] = FIXED(m->m[1][0] * v->vy);
	m->m[1][1] = FIXED(m->m[1][1] * v->vy);
	m->m[1][2] = FIXED(m->m[1][2] * v->vy);
	m->m[2][0] = FIXED(m->m[2][0] * v->vz);
	m->m[2][1] = FIXED(m->m[2][1] * v->vz);
	m->m[2][2] = FIXED(m->m[2][2] * v->vz);
	return m;
}

void SetDQA(int iDQA)
{
	CTC2(*(u32 *)&iDQA, 27);
}

void SetDQB(int iDQB)
{
	CTC2(*(u32 *)&iDQB, 28);
}

void SetFogNear(int a, int h)
{
	// Error division by 0
	assert(h != 0);
	int depthQ = -(((a << 2) + a) << 6);
	assert(h != -1 && depthQ != 0x8000);
	SetDQA(depthQ / h);
	SetDQB(20971520);
}

void SetFogNearFar(int a, int b, int h)
{
	if (b - a < 100)
	{
		return;
	}

	assert((b - a));
	assert((b - a) != -1 && (-a * b) != 32768);
	assert((b - a));
	assert((b - a) != -1 && (b << 12) != 32768);
	assert(h != 0);
	assert(h != -1 && (((-a * b) / (b - a)) << 8) != 32768);

	int dqa = (-a * b / (b - a) << 8) / h;

	SetDQA(MAX(MIN(dqa, 32767), -32767));
	SetDQB((b << 12) / (b - a) << 12);
}

int rsin(int a)
{
	if (a < 0)
	{
		return -rcossin_tbl[(-a & 0xfffU) * 2];
	}

	return rcossin_tbl[(a & 0xfffU) * 2];
}

int rcos(int a)
{
	if (a < 0)
	{
		return rcossin_tbl[(-a & 0xfffU) * 2 + 1];
	}

	return rcossin_tbl[(a & 0xfffU) * 2 + 1];
}

int ratan2(int y, int x)
{
	int v;
	u32 ang;
	int xlt0, ylt0;

	xlt0 = x < 0;
	ylt0 = y < 0;

	if (x == 0 && y == 0)
	{
		return 0;
	}

	if (x < 0)
	{
		x = -x;
	}

	if (y < 0)
	{
		y = -y;
	}

	if (y < x)
	{
		if (((u32)y & 0x7fe00000U) == 0)
		{
			ang = (y << 10) / x;
		}
		else
		{
			ang = y / (x >> 10);
		}

		v = ratan_tbl[ang];
	}
	else
	{
		if (((u32)x & 0x7fe00000U) == 0)
		{
			ang = (x << 10) / y;
		}
		else
		{
			ang = x / (y >> 10);
		}

		v = 1024 - ratan_tbl[ang];
	}

	if (xlt0)
	{
		v = 2048 - v;
	}

	if (ylt0)
	{
		v = -v;
	}

	return v;
}

int SquareRoot0(int a)
{
	// correct Psy-Q implementation
	int idx;
	int lzcs;
	lzcs = gte_leadingzerocount(a);

	if (lzcs == 32)
	{
		return 0;
	}

	lzcs &= 0xfffffffe;

	if ((lzcs - 24) < 0)
	{
		idx = a >> (24 - lzcs);
	}
	else
	{
		idx = a << (lzcs - 24);
	}

	return SQRT[idx - 64] << ((31 - lzcs) >> 1) >> 12;
}
