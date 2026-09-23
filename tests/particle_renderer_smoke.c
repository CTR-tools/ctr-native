// NOTE(aalhendi): Include the production source once so its native sdata
// binding has one definition. Only the renderer's reachable sections link.
#include "../game/Particle.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Data data;
struct sData sdata_static;
u8 *gCTRNativeScratchpadBase;

static u8 scratchpad[0x400];
static u32 gteData[32];
static u32 gteControl[32];
static int corners;
static int lightTransforms;
static int quadCorners;
static int lineTransforms;

// NOTE(aalhendi): Deterministic COP2 outputs isolate packet construction and
// OT linking from GTE arithmetic. The GPU link bridge is the real one.
void PushBuffer_SetPsyqGeom(struct PushBuffer *pb)
{
	(void)pb;
}

void MTC2(u32 value, int reg)
{
	gteData[reg] = value;
}

void CTC2(u32 value, int reg)
{
	gteControl[reg] = value;
}

u32 MFC2(int reg)
{
	return gteData[reg];
}

int MFC2_S(int reg)
{
	return (s32)gteData[reg];
}

int doCOP2(int op)
{
	if (op == 0x04a6012)
	{
		lightTransforms++;
		gteData[25] = 0;
		gteData[26] = 0;
		gteData[27] = 1024;
	}
	else if (op == 0x0180001)
	{
		quadCorners++;
		gteData[14] = (u32)(100 + corners) | ((u32)(200 + corners) << 16);
		gteData[19] = 1024;
		corners++;
	}
	else if (op == 0x0280030)
	{
		lineTransforms++;
		gteData[12] = 10 | (20u << 16);
		gteData[13] = 30 | (40u << 16);
		gteData[17] = 1024;
	}
	else
	{
		fprintf(stderr, "unexpected GTE op %x\n", op);
		abort();
	}
	return 0;
}

#define CHECK(condition)                                                 \
	do                                                                   \
	{                                                                    \
		if (!(condition))                                                \
		{                                                                \
			fprintf(stderr, "failed: %s at %d\n", #condition, __LINE__); \
			return 1;                                                    \
		}                                                                \
	} while (0)

int main(void)
{
	static struct GameTracker game;
	static struct DB backBuffer;
	static struct PushBuffer pushBuffer;
	static struct Particle particle;
	static struct IconGroup iconGroup;
	static struct Icon icon;
	static u32 prim[128];
	static u32 ot[0x400];
	struct ParticleSpecialPacket *line;
	POLY_FT4 *quad;
	int i;

	gCTRNativeScratchpadBase = scratchpad;
	sdata_static.gGT = &game;
	game.backBuffer = &backBuffer;
	game.numParticles = 1;
	backBuffer.primMem.cursor = prim;
	backBuffer.primMem.guardEnd = prim + 128;
	pushBuffer.ptrOT = ot;
	pushBuffer.matrix_ViewProj.m[0][0] = 0x111;
	pushBuffer.matrix_ViewProj.m[0][1] = 0x222;
	for (i = 0; i < 0x400; i++)
		ot[i] = 0x00ffffff;
	NativeGpuLinks_Reset();
	CHECK(NativeGpuLinks_RegisterRange(prim, sizeof(prim), NULL));

	particle.driverID = -1;
	particle.renderDepthLimit = 0x400;
	particle.ptrIconGroup = &iconGroup;
	particle.ptrIconArray = &icon;
	iconGroup.numIcons = 1;
	icon.texLayout.u0 = 2;
	icon.texLayout.u1 = 17;
	icon.texLayout.u2 = 2;
	icon.texLayout.u3 = 17;
	icon.texLayout.v0 = 4;
	icon.texLayout.v2 = 20;
	Particle_RenderList(&pushBuffer, &particle);
	quad = (POLY_FT4 *)prim;
	CHECK(backBuffer.primMem.cursor == prim + 10);
	CHECK(quad->tag == 0x09ffffff);
	CHECK(quad->code == 0x2d);
	CHECK(quad->x0 == 100 && quad->y0 == 200);
	CHECK(quad->x3 == 103 && quad->y3 == 203);
	CHECK(quad->u0 == 2 && quad->u1 == 17);
	CHECK(gteControl[8] == (0x111u | (0x222u << 16)));
	CHECK(gteControl[0] == 0x2000);
	CHECK(ot[4] == NativeGpuLinks_FromHostPointer(prim));
	CHECK(NativeGpuLinks_ToHostPointer(ot[4]) == prim);
	CHECK(lightTransforms == 1 && quadCorners == 4 && lineTransforms == 0);

	memset(prim, 0, sizeof(prim));
	backBuffer.primMem.cursor = prim;
	particle.flagsSetColor = PARTICLE_SET_COLOR_FLAG_SPECIAL_LINE;
	particle.axis[PARTICLE_AXIS_ICON_FRAME_OR_LINE_COLOR].startVal = 0x11223344;
	ot[4] = 0x00ffffff;
	Particle_RenderList(&pushBuffer, &particle);
	line = (struct ParticleSpecialPacket *)prim;
	CHECK(backBuffer.primMem.cursor == prim + 7);
	CHECK(line->tag == 0x06ffffff);
	CHECK(line->drawMode == PARTICLE_GPU_DRAWMODE_BASE);
	CHECK(line->line.color0AndCode == 0x51000000);
	CHECK(line->line.color1 == 0x11223344);
	CHECK(line->line.xy0 == (10u | (20u << 16)));
	CHECK(line->line.xy1 == (30u | (40u << 16)));
	CHECK(ot[4] == NativeGpuLinks_FromHostPointer(prim));
	CHECK(lightTransforms == 2 && quadCorners == 4 && lineTransforms == 1);

	memset(prim, 0, sizeof(prim));
	backBuffer.primMem.cursor = prim;
	backBuffer.primMem.guardEnd = prim + 10;
	ot[4] = 0x00ffffff;
	Particle_RenderList(&pushBuffer, &particle);
	CHECK(backBuffer.primMem.cursor == prim);
	CHECK(prim[0] == 0);
	CHECK(ot[4] == 0x00ffffff);
	CHECK(lightTransforms == 2 && quadCorners == 4 && lineTransforms == 1);

	puts("Particle renderer smoke passed: FT4, special line, and guard");
	return 0;
}
