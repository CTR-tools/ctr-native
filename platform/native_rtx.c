// MultiTrack: real-time ray tracing bridge (game side, 32-bit).
// See include/platform/native_rtx.h and rtx/rtx_shared.h.

#include "platform/native_rtx.h"
#include "../rtx/rtx_shared.h"

#include <SDL3/SDL.h>

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

extern char **environ;
// not declared under -std=c99 without _POSIX_C_SOURCE (unity build includes headers first)
int ftruncate(int fd, off_t length);
int kill(pid_t pid, int sig);

const u16 *NativeRenderer_RtxVRAM(void);

int g_rtxCapture;

#define RTX_MAX_RANGES  8192
#define RTX_CAP_BITS    16
#define RTX_CAP_SIZE    (1 << RTX_CAP_BITS)
#define RTX_MAX_TEXSRC  RTX_MAX_TEX_SLOTS
#define RTX_MAX_EMITTERS 1024

struct RtxRange
{
	u32 start, end;
	u16 tag;
	u16 serial;
	u8 allowSemi; // wheels: semi-transparent code, but colour-keyed sprites
	u8 mode;      // RtxInstMode
};

enum RtxInstMode
{
	RTX_INST_CAPTURE, // solid model: ray traced geometry
	RTX_INST_GLOW,    // transparent glowing model (turbo flames, fire jets): GL draws it, it lights the scene
	RTX_INST_GL,      // ghosts and other transparent models: GL only
};

// something glowing (additive blend) seen this frame; clustered into point lights
struct RtxEmitter
{
	float pos[3];
	float color[3]; // weighted sum
	float weight;
	float radius;
};

struct RtxCapEntry
{
	u32 sxy;
	u32 serial;
	u32 gen;
	float v[3];
};

static struct
{
	int fd;
	char shmName[64];
	u8 *base;
	struct RtxHeader *hdr;
	pid_t rendererPid;
	int enabled;   // F7
	int debugView; // F6

	// per frame
	u32 gen;
	int cameraValid;
	float rInv[9];
	float camT[3];
	struct RtxRange ranges[RTX_MAX_RANGES];
	int numRanges;
	int rangesSorted;
	int tagStack[4];
	u32 tagStart[4];
	int tagDepth;
	u16 serial;
	float instScale; // true view / GTE view of the current instance
	int allowSemi;
	int instMode;
	int lightMode;
	int camForCapture;
	int skyPhase;
	int numDyn;
	struct RtxEmitter emitters[RTX_MAX_EMITTERS];
	int numEmitters;

	// level
	void *levelPtr;
	void *meshPtr;
	int levelID;
	// one per slot: ptr_texture_mid value (IconGroup4, | 1 = AnimTex), or a plain
	// TextureLayout | 2 (the water env map)
	u32 texSrc[RTX_MAX_TEXSRC];
	int numTexSlots;
	int levelValid;

	struct RtxCapEntry cap[RTX_CAP_SIZE];

	// packets captured into dynamic triangles this frame (GL skips them)
	u32 capturedKey[1 << 15];
	u32 capturedGen[1 << 15];
} s_rtx;

#define RTX_HDR (s_rtx.hdr)

static struct RtxLevelVert *Rtx_LevelVerts(void) { return (struct RtxLevelVert *)(s_rtx.base + RTX_OFF_LEVEL_VERTS); }
static struct RtxLevelTri *Rtx_LevelTris(void) { return (struct RtxLevelTri *)(s_rtx.base + RTX_OFF_LEVEL_TRIS); }
static struct RtxTexSlot *Rtx_TexSlots(void) { return (struct RtxTexSlot *)(s_rtx.base + RTX_OFF_TEX_SLOTS); }
static struct RtxDynTri *Rtx_DynTris(void) { return (struct RtxDynTri *)(s_rtx.base + RTX_OFF_DYN_TRIS); }

void Rtx_Init(void)
{
	const char *env = SDL_getenv("CTR_RTX");
	if ((env != NULL) && (env[0] == '0'))
	{
		printf("[RTX] disabled (CTR_RTX=0)\n");
		return;
	}

	snprintf(s_rtx.shmName, sizeof(s_rtx.shmName), "%s-%d", RTX_SHM_NAME, (int)getpid());
	s_rtx.fd = shm_open(s_rtx.shmName, O_CREAT | O_RDWR | O_TRUNC, 0600);
	if (s_rtx.fd < 0)
	{
		printf("[RTX] shm_open failed: %s\n", strerror(errno));
		return;
	}
	if (ftruncate(s_rtx.fd, RTX_SHM_SIZE) != 0)
	{
		printf("[RTX] ftruncate %u failed: %s\n", (unsigned)RTX_SHM_SIZE, strerror(errno));
		return;
	}
	void *map = mmap(NULL, RTX_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, s_rtx.fd, 0);
	if (map == MAP_FAILED)
	{
		printf("[RTX] mmap failed: %s\n", strerror(errno));
		return;
	}
	s_rtx.base = (u8 *)map;
	s_rtx.hdr = (struct RtxHeader *)map;
	memset(s_rtx.hdr, 0, sizeof(*s_rtx.hdr));
	RTX_HDR->magic = RTX_MAGIC;
	RTX_HDR->version = RTX_VERSION;
	RTX_HDR->gamePid = (u32)getpid();
	RTX_HDR->redirectBase = -1;

	char exe[1024];
	snprintf(exe, sizeof(exe), "%sctr_rtx", SDL_GetBasePath());
	char *argv[] = {exe, s_rtx.shmName, NULL};
	pid_t pid;
	int err = posix_spawn(&pid, exe, NULL, NULL, argv, environ);
	if (err != 0)
	{
		printf("[RTX] could not start %s: %s (ray tracing off)\n", exe, strerror(err));
		shm_unlink(s_rtx.shmName);
		return;
	}
	s_rtx.rendererPid = pid;
	s_rtx.enabled = 1;
	printf("[RTX] renderer %s started (pid %d): F7 ray tracing on/off, F5 hybrid/full lighting, F6 debug views\n", exe, (int)pid);
}

void Rtx_Shutdown(void)
{
	if (RTX_HDR != NULL)
	{
		RTX_HDR->quit = 1;
	}
	if (s_rtx.shmName[0])
	{
		shm_unlink(s_rtx.shmName);
	}
}

void Rtx_HandleKey(int scancode)
{
	if (scancode == SDL_SCANCODE_F7)
	{
		s_rtx.enabled ^= 1;
		printf("[RTX] ray tracing %s\n", s_rtx.enabled ? "on" : "off");
	}
	else if (scancode == SDL_SCANCODE_F5)
	{
		s_rtx.lightMode ^= 1;
		printf("[RTX] lighting: %s\n", s_rtx.lightMode ? "hybrid (track's own lighting + ray traced extras)" : "full relight");
	}
	else if (scancode == SDL_SCANCODE_F6)
	{
		s_rtx.debugView = (s_rtx.debugView + 1) % 5;
		static const char *names[] = {"final", "albedo", "normal", "direct", "indirect"};
		printf("[RTX] debug view: %s\n", names[s_rtx.debugView]);
	}
}

// ---- per-frame capture -----------------------------------------------------

void Rtx_FrameBegin(void)
{
	s_rtx.gen++;
	if (s_rtx.gen == 0)
	{
		s_rtx.gen = 1;
	}
	s_rtx.cameraValid = 0;
	s_rtx.numRanges = 0;
	s_rtx.rangesSorted = 1;
	s_rtx.tagDepth = 0;
	s_rtx.skyPhase = 1;
	s_rtx.numDyn = 0;
	s_rtx.numEmitters = 0;
	s_rtx.camForCapture = 0;
	g_rtxCapture = 0;
}

void Rtx_BeginTag(int tag, struct PrimMem *primMem)
{
	if ((RTX_HDR == NULL) || (s_rtx.tagDepth >= 4))
	{
		return;
	}
	s_rtx.tagStack[s_rtx.tagDepth] = tag;
	s_rtx.tagStart[s_rtx.tagDepth] = (u32)primMem->cursor;
	s_rtx.tagDepth++;
	if (tag == RTX_TAG_INSTANCE)
	{
		s_rtx.serial++;
		if (s_rtx.serial == 0)
		{
			s_rtx.serial = 1;
		}
		g_rtxCapture = 1;
	}
}

static int Rtx_CapturePacket(u32 *words, int numWords, u16 serial, int allowSemi);
static void Rtx_EmitterPacket(u32 *words, int numWords, u16 serial, int anyBlend);
static void Rtx_MarkCaptured(u32 packet);

void Rtx_EndTag(struct PrimMem *primMem)
{
	if ((RTX_HDR == NULL) || (s_rtx.tagDepth <= 0))
	{
		return;
	}
	s_rtx.tagDepth--;
	int tag = s_rtx.tagStack[s_rtx.tagDepth];
	u32 start = s_rtx.tagStart[s_rtx.tagDepth];
	u32 end = (u32)primMem->cursor;
	g_rtxCapture = 0;

	if ((end <= start) || (s_rtx.numRanges >= RTX_MAX_RANGES))
	{
		return;
	}
	struct RtxRange *r = &s_rtx.ranges[s_rtx.numRanges++];
	if ((s_rtx.numRanges > 1) && (start < r[-1].start))
	{
		s_rtx.rangesSorted = 0;
	}
	r->start = start;
	r->end = end;
	r->tag = (u16)tag;
	r->serial = (tag == RTX_TAG_INSTANCE) ? s_rtx.serial : 0;
	r->allowSemi = (u8)s_rtx.allowSemi;
	r->mode = (u8)s_rtx.instMode;
	s_rtx.allowSemi = 0;
	s_rtx.instMode = RTX_INST_CAPTURE;

	// model packets are allocated back to back: decode them now, so the ray
	// traced image can be rendered before the GL overlay pass draws over it
	if ((tag == RTX_TAG_INSTANCE) && s_rtx.enabled && RTX_HDR->rendererReady)
	{
		u32 *p = (u32 *)start;
		while ((u32)p < end)
		{
			int len = getlen(p);
			if ((len == 0) || (len > 32) || ((u32)(p + 1 + len) > end))
			{
				break;
			}
			if (r->mode == RTX_INST_GLOW)
			{
				Rtx_EmitterPacket(p + 1, len, r->serial, 1);
			}
			else if (r->mode == RTX_INST_GL)
			{
			}
			else if (Rtx_CapturePacket(p + 1, len, r->serial, r->allowSemi))
			{
				Rtx_MarkCaptured((u32)p);
			}
			else
			{
				Rtx_EmitterPacket(p + 1, len, r->serial, 0);
			}
			p += 1 + len;
		}
	}
}

static u32 Rtx_PtrHash(u32 p)
{
	u32 h = p * 0x9E3779B1u;
	return (h ^ (h >> 16)) & ((1 << 15) - 1);
}

static void Rtx_MarkCaptured(u32 packet)
{
	u32 h = Rtx_PtrHash(packet);
	for (int probe = 0; probe < 64; probe++)
	{
		u32 i = (h + probe) & ((1 << 15) - 1);
		if (s_rtx.capturedGen[i] != s_rtx.gen)
		{
			s_rtx.capturedGen[i] = s_rtx.gen;
			s_rtx.capturedKey[i] = packet;
			return;
		}
	}
}

static int Rtx_IsCaptured(u32 packet)
{
	u32 h = Rtx_PtrHash(packet);
	for (int probe = 0; probe < 64; probe++)
	{
		u32 i = (h + probe) & ((1 << 15) - 1);
		if (s_rtx.capturedGen[i] != s_rtx.gen)
		{
			return 0;
		}
		if (s_rtx.capturedKey[i] == packet)
		{
			return 1;
		}
	}
	return 0;
}

static void Rtx_Invert3(const float *m, float *out)
{
	float a = m[0], b = m[1], c = m[2], d = m[3], e = m[4], f = m[5], g = m[6], h = m[7], i = m[8];
	float A = e * i - f * h, B = -(d * i - f * g), C = d * h - e * g;
	float det = a * A + b * B + c * C;
	float inv = (fabsf(det) > 1e-12f) ? 1.0f / det : 0.0f;
	out[0] = A * inv;
	out[1] = -(b * i - c * h) * inv;
	out[2] = (b * f - c * e) * inv;
	out[3] = B * inv;
	out[4] = (a * i - c * g) * inv;
	out[5] = -(a * f - c * d) * inv;
	out[6] = C * inv;
	out[7] = -(a * h - b * g) * inv;
	out[8] = (a * e - b * d) * inv;
}

// camera for view -> world of everything captured this frame (models are drawn
// before the level, whose hook sets the same camera again)
static void Rtx_CaptureCamera(struct PushBuffer *pb)
{
	if (s_rtx.camForCapture)
	{
		return;
	}
	const MATRIX *m = &pb->matrix_ViewProj;
	float camR[9];
	for (int r = 0; r < 3; r++)
	{
		for (int c = 0; c < 3; c++)
		{
			camR[r * 3 + c] = m->m[r][c] / 4096.0f;
		}
		s_rtx.camT[r] = (float)m->t[r];
	}
	Rtx_Invert3(camR, s_rtx.rInv);
	s_rtx.camForCapture = 1;
}

// Model instances project through an MVP whose view space is uniformly scaled
// (near instances x4, DRAW_HUGE /4, see RenderBucket_AdjustViewPositionForMvp);
// the scale falls out of the MVP translation vs the instance's true view position
void Rtx_BeginInstance(struct PrimMem *primMem, struct Instance *inst, struct PushBuffer *pb, const s32 *mvpT)
{
	struct GameTracker *gGT = sdata->gGT;
	if ((RTX_HDR == NULL) || (pb != &gGT->pushBuffer[0]) || (inst->flags & SCREENSPACE_INSTANCE))
	{
		Rtx_BeginTag(RTX_TAG_NONE, primMem); // HUD models, other players' views: GL
		return;
	}
	// transparent models stay GL-drawn (as on PS1); glowing ones also light the scene
	if ((inst->flags & GHOST_DRAW_TRANSPARENT) == GHOST_DRAW_TRANSPARENT)
	{
		s_rtx.instMode = RTX_INST_GL;
	}
	else if (inst->flags & (DRAW_TRANSPARENT | DEPTH_FADE))
	{
		s_rtx.instMode = RTX_INST_GLOW;
	}
	else
	{
		s_rtx.instMode = RTX_INST_CAPTURE;
	}

	const MATRIX *m = &pb->matrix_ViewProj;
	float rel[3] = {(float)(inst->matrix.t[0] - pb->pos.x), (float)(inst->matrix.t[1] - pb->pos.y), (float)(inst->matrix.t[2] - pb->pos.z)};
	float vTrue[3], dotTT = 0.0f, dotGT = 0.0f;
	for (int r = 0; r < 3; r++)
	{
		vTrue[r] = (m->m[r][0] * rel[0] + m->m[r][1] * rel[1] + m->m[r][2] * rel[2]) / 4096.0f;
		dotTT += vTrue[r] * vTrue[r];
		dotGT += vTrue[r] * (float)mvpT[r];
	}
	s_rtx.instScale = (dotTT > 1.0f && dotGT > 0.0f) ? dotTT / dotGT : 1.0f;
	Rtx_CaptureCamera(pb);
	Rtx_BeginTag(RTX_TAG_INSTANCE, primMem);
}

void Rtx_BeginScaled(struct PrimMem *primMem, struct PushBuffer *pb, float viewScale)
{
	if ((RTX_HDR == NULL) || (pb != &sdata->gGT->pushBuffer[0]))
	{
		Rtx_BeginTag(RTX_TAG_NONE, primMem);
		return;
	}
	s_rtx.instScale = viewScale;
	s_rtx.allowSemi = 1;
	s_rtx.instMode = RTX_INST_CAPTURE;
	Rtx_CaptureCamera(pb);
	Rtx_BeginTag(RTX_TAG_INSTANCE, primMem);
}

static u32 Rtx_CapHash(u32 sxy, u32 serial)
{
	u32 h = sxy * 0x9E3779B1u ^ serial * 0x85EBCA77u;
	h ^= h >> 15;
	return h & (RTX_CAP_SIZE - 1);
}

void Rtx_OnProject(u32 sxy, s32 mac1, s32 mac2, s32 mac3, int sf)
{
	u32 h = Rtx_CapHash(sxy, s_rtx.serial);
	float scale = (sf ? 1.0f : (1.0f / 4096.0f)) * s_rtx.instScale;

	for (int probe = 0; probe < 16; probe++)
	{
		struct RtxCapEntry *e = &s_rtx.cap[(h + probe) & (RTX_CAP_SIZE - 1)];
		if ((e->gen != s_rtx.gen) || ((e->sxy == sxy) && (e->serial == s_rtx.serial)))
		{
			e->gen = s_rtx.gen;
			e->sxy = sxy;
			e->serial = s_rtx.serial;
			e->v[0] = mac1 * scale;
			e->v[1] = mac2 * scale;
			e->v[2] = mac3 * scale;
			return;
		}
	}
}

static const float *Rtx_CapLookup(u32 sxy, u32 serial)
{
	u32 h = Rtx_CapHash(sxy, serial);

	for (int probe = 0; probe < 16; probe++)
	{
		struct RtxCapEntry *e = &s_rtx.cap[(h + probe) & (RTX_CAP_SIZE - 1)];
		if (e->gen != s_rtx.gen)
		{
			return NULL;
		}
		if ((e->sxy == sxy) && (e->serial == serial))
		{
			return e->v;
		}
	}
	return NULL;
}

// ---- level upload ----------------------------------------------------------

// QuadBlock hi-LOD faces as (TL, TR, BL, BR) quadblock vertex slots, from
// overlay 226 (sDrawLevelOvr1PGridFaceIndices)
static const int sRtxGridFace[4][4] = {
    {0, 4, 5, 6},
    {4, 1, 6, 7},
    {5, 6, 2, 8},
    {6, 7, 8, 3},
};

static int Rtx_TexSlotFor(u32 ptr)
{
	if (ptr == 0)
	{
		return -1;
	}
	// small levels: linear search on a hash of recent slots
	static u32 hashKey[4096];
	static u16 hashVal[4096];
	static u32 hashGen[4096];
	static u32 gen;
	if (s_rtx.numTexSlots == 0)
	{
		gen++;
	}
	u32 h = (ptr * 0x9E3779B1u) >> 20;
	for (int probe = 0; probe < 4096; probe++)
	{
		u32 i = (h + probe) & 4095;
		if (hashGen[i] != gen)
		{
			if (s_rtx.numTexSlots >= (int)RTX_MAX_TEXSRC)
			{
				return -1;
			}
			hashGen[i] = gen;
			hashKey[i] = ptr;
			hashVal[i] = (u16)s_rtx.numTexSlots;
			s_rtx.texSrc[s_rtx.numTexSlots] = ptr;
			return s_rtx.numTexSlots++;
		}
		if (hashKey[i] == ptr)
		{
			return hashVal[i];
		}
	}
	return -1;
}

static void Rtx_RefreshTexSlots(void)
{
	struct RtxTexSlot *slots = Rtx_TexSlots();

	for (int i = 0; i < s_rtx.numTexSlots; i++)
	{
		u32 ptr = s_rtx.texSrc[i];
		const struct IconGroup4 *group;
		const struct TextureLayout *t;
		if (ptr & 2)
		{
			t = (const struct TextureLayout *)(ptr & ~3u);
			goto haveLayout;
		}
		if (ptr & 1)
		{
			group = *(const struct IconGroup4 **)(ptr - 1); // AnimTex: active frame
		}
		else
		{
			group = (const struct IconGroup4 *)ptr;
		}
		if (group == NULL)
		{
			continue;
		}
		t = &group->near;
	haveLayout:;
		struct RtxTexSlot *s = &slots[i];
		s->uv[0][0] = t->u0;
		s->uv[0][1] = t->v0;
		s->uv[1][0] = t->u1;
		s->uv[1][1] = t->v1;
		s->uv[2][0] = t->u2;
		s->uv[2][1] = t->v2;
		s->uv[3][0] = t->u3;
		s->uv[3][1] = t->v3;
		s->clut = t->clut;
		s->tpage = t->tpage;
	}
	RTX_HDR->numTexSlots = (u32)s_rtx.numTexSlots;
}

static void Rtx_UploadLevel(struct Level *level)
{
	struct mesh_info *mesh = level->ptr_mesh_info;
	struct RtxLevelVert *verts = Rtx_LevelVerts();
	struct RtxLevelTri *tris = Rtx_LevelTris();
	int numVerts = mesh->numVertex;
	int numTris = 0;

	if (numVerts > (int)RTX_MAX_LEVEL_VERTS)
	{
		numVerts = RTX_MAX_LEVEL_VERTS;
	}
	for (int i = 0; i < numVerts; i++)
	{
		const struct LevVertex *v = &mesh->ptrVertexArray[i];
		verts[i].pos[0] = v->pos.x;
		verts[i].pos[1] = v->pos.y;
		verts[i].pos[2] = v->pos.z;
		verts[i].rgba = (u32)v->color_hi[0] | ((u32)v->color_hi[1] << 8) | ((u32)v->color_hi[2] << 16) | (0xffu << 24);
	}

	// water: the quadblocks of BSP leaves flagged BSP_LEAF_FLAG_WATER
	static u8 isWater[1 << 16];
	int numQuads = mesh->numQuadBlock < (1 << 16) ? mesh->numQuadBlock : (1 << 16);
	memset(isWater, 0, sizeof(isWater));
	for (int b = 0; (mesh->bspRoot != NULL) && (b < mesh->numBspNodes); b++)
	{
		const struct BSP *bsp = &mesh->bspRoot[b];
		if (((bsp->flag & 1) != 0) && ((bsp->flag & BSP_LEAF_FLAG_WATER) != 0))
		{
			int first = (int)(bsp->data.leaf.ptrQuadBlockArray - mesh->ptrQuadBlockArray);
			for (int k = 0; k < bsp->data.leaf.numQuads; k++)
			{
				if ((first + k >= 0) && (first + k < numQuads))
				{
					isWater[first + k] = 1;
				}
			}
		}
	}

	s_rtx.numTexSlots = 0; // also resets Rtx_TexSlotFor's lookup
	int waterSlot = (level->ptr_tex_waterEnvMap != NULL) ? Rtx_TexSlotFor((u32)level->ptr_tex_waterEnvMap | 2) : -1;
	int skippedInvisible = 0;

	for (int q = 0; q < numQuads; q++)
	{
		const struct QuadBlock *block = &mesh->ptrQuadBlockArray[q];
		// invisible walls and triggers: quadFlags 0x8000 (ctr-tools "invisible";
		// such blocks also have no draw order), or no texture and not water (water
		// is drawn with the level's env map instead of its own textures)
		int untextured = !block->ptr_texture_mid[0] && !block->ptr_texture_mid[1] && !block->ptr_texture_mid[2] && !block->ptr_texture_mid[3];
		if ((*(const u16 *)&block->quadFlags & 0x8000) || (untextured && !isWater[q]))
		{
			skippedInvisible++;
			continue;
		}
		u32 drawOrderLow = block->draw_order_low;

		for (int f = 0; f < 4; f++)
		{
			u32 faceBits = (drawOrderLow >> (8 + f * 5)) & 0x1f;
			u32 tableWord = R226.scratchInitTable[faceBits < OVR226_SCRATCH_INIT_WORD_COUNT ? faceBits : 0];
			int slotVert[4];
			for (int s = 0; s < 4; s++)
			{
				u32 shift = (tableWord >> ((3 - s) * 8)) & 0x1f;
				int k = (24 - (int)shift) / 8;
				slotVert[s] = block->index[sRtxGridFace[f][k & 3]];
			}

			int texSlot = isWater[q] && (waterSlot >= 0) ? waterSlot : Rtx_TexSlotFor((u32)block->ptr_texture_mid[f]);
			static const int triSlots[2][3] = {{0, 1, 2}, {1, 3, 2}};
			for (int t = 0; t < 2; t++)
			{
				if (numTris >= (int)RTX_MAX_LEVEL_TRIS)
				{
					break;
				}
				struct RtxLevelTri *tri = &tris[numTris];
				int ok = 1;
				for (int c = 0; c < 3; c++)
				{
					int vi = slotVert[triSlots[t][c]];
					ok &= (vi < numVerts);
					tri->v[c] = (u16)vi;
					tri->corner[c] = (u8)triSlots[t][c];
				}
				if (!ok || (tri->v[0] == tri->v[1]) || (tri->v[1] == tri->v[2]) || (tri->v[0] == tri->v[2]))
				{
					continue;
				}
				tri->texSlot = (u16)(texSlot < 0 ? 0xffff : texSlot);
				tri->flags = (texSlot >= 0 ? RTX_TRI_TEXTURED : 0) | ((drawOrderLow & 0x80000000u) ? RTX_TRI_DOUBLE_SIDED : 0) |
				             (isWater[q] ? RTX_TRI_WATER : 0);
				numTris++;
			}
		}
	}

	struct GameTracker *gGT = sdata->gGT;
	struct MTEntry *custom = MT_ActiveEntry();
	RTX_HDR->numLevelVerts = (u32)numVerts;
	RTX_HDR->numLevelTris = (u32)numTris;
	RTX_HDR->levelID = gGT->levelID;
	RTX_HDR->redirectBase = custom ? custom->bigBase : -1;
	if (custom)
	{
		snprintf(RTX_HDR->lightsKey, sizeof(RTX_HDR->lightsKey), "custom-%d", custom->bigBase);
	}
	else
	{
		snprintf(RTX_HDR->lightsKey, sizeof(RTX_HDR->lightsKey), "stock-%d", gGT->levelID);
	}
	Rtx_RefreshTexSlots();
	RTX_HDR->levelGen++;

	s_rtx.levelPtr = level;
	s_rtx.meshPtr = mesh;
	s_rtx.levelID = gGT->levelID;
	s_rtx.levelValid = 1;
	printf("[RTX] level %d (%s): %d verts, %d tris, %d texture slots, %d invisible quads skipped\n", gGT->levelID, RTX_HDR->lightsKey, numVerts,
	       numTris, s_rtx.numTexSlots, skippedInvisible);
}

void Rtx_OnLevelCamera(struct PushBuffer *pb)
{
	struct GameTracker *gGT = sdata->gGT;
	struct Level *level = gGT->level1;

	// gameplay only: menus and cutscenes keep the PS1 look
	if ((RTX_HDR == NULL) || (level == NULL) || (level->ptr_mesh_info == NULL) || (gGT->numPlyrCurrGame != 1) ||
	    ((gGT->gameMode1 & (MAIN_MENU | GAME_CUTSCENE | END_OF_RACE)) != 0) || (gGT->levelID >= GEM_STONE_VALLEY) || (sdata->pause_state != 0))
	{
		return;
	}
	if ((s_rtx.levelPtr != level) || (s_rtx.meshPtr != level->ptr_mesh_info) || (s_rtx.levelID != gGT->levelID) || !s_rtx.levelValid)
	{
		Rtx_UploadLevel(level);
	}

	const MATRIX *m = &pb->matrix_ViewProj;
	for (int r = 0; r < 3; r++)
	{
		for (int c = 0; c < 3; c++)
		{
			RTX_HDR->camR[r * 3 + c] = m->m[r][c] / 4096.0f;
		}
		RTX_HDR->camT[r] = (float)m->t[r];
		s_rtx.camT[r] = (float)m->t[r];
	}
	Rtx_Invert3(RTX_HDR->camR, s_rtx.rInv);
	RTX_HDR->projH = (float)pb->distanceToScreen_PREV;
	RTX_HDR->vpX = pb->rect.x;
	RTX_HDR->vpY = pb->rect.y;
	RTX_HDR->vpW = pb->rect.w;
	RTX_HDR->vpH = pb->rect.h;
	s_rtx.cameraValid = 1;
}

// ---- GPU side ----------------------------------------------------------------

int Rtx_FrameActive(void)
{
	return (RTX_HDR != NULL) && s_rtx.enabled && RTX_HDR->rendererReady && s_rtx.cameraValid && s_rtx.levelValid;
}

static int Rtx_RangeCmp(const void *a, const void *b)
{
	const struct RtxRange *ra = a, *rb = b;
	return (ra->start > rb->start) - (ra->start < rb->start);
}

static const struct RtxRange *Rtx_FindRange(u32 addr)
{
	if (!s_rtx.rangesSorted)
	{
		qsort(s_rtx.ranges, s_rtx.numRanges, sizeof(s_rtx.ranges[0]), Rtx_RangeCmp);
		s_rtx.rangesSorted = 1;
	}
	int lo = 0, hi = s_rtx.numRanges - 1;
	while (lo <= hi)
	{
		int mid = (lo + hi) >> 1;
		const struct RtxRange *r = &s_rtx.ranges[mid];
		if (addr < r->start)
		{
			hi = mid - 1;
		}
		else if (addr >= r->end)
		{
			lo = mid + 1;
		}
		else
		{
			return r;
		}
	}
	return NULL;
}

static void Rtx_ViewToWorld(const float *v, float *out)
{
	float x = v[0] - s_rtx.camT[0], y = v[1] - s_rtx.camT[1], z = v[2] - s_rtx.camT[2];
	const float *m = s_rtx.rInv;
	out[0] = m[0] * x + m[1] * y + m[2] * z;
	out[1] = m[3] * x + m[4] * y + m[5] * z;
	out[2] = m[6] * x + m[7] * y + m[8] * z;
}

// Decodes the GPU polygon commands of one packet into world-space triangles.
// Returns 1 when every command was captured (GL can skip the packet).
static int Rtx_CapturePacket(u32 *words, int numWords, u16 serial, int allowSemi)
{
	int startDyn = s_rtx.numDyn;
	int i = 0;
	int polys = 0;

	while (i < numWords)
	{
		u32 cmd = words[i];
		u32 code = cmd >> 24;

		if ((code == 0) || ((code >= 0xe1) && (code <= 0xe6)))
		{
			i++;
			continue;
		}
		if ((code < 0x20) || (code >= 0x40) || ((code & 2) && !(allowSemi && (code & 4))))
		{
			goto fail; // lines, sprites, semi-transparent: leave to GL
		}

		int gouraud = (code & 0x10) != 0;
		int quad = (code & 0x08) != 0;
		int textured = (code & 0x04) != 0;
		int raw = (code & 0x01) != 0;
		int n = quad ? 4 : 3;
		int size = 1 + n * (1 + textured) + (gouraud ? n - 1 : 0);
		if (i + size > numWords)
		{
			goto fail;
		}

		float pos[4][3];
		u8 uv[4][2];
		u8 col[4][4];
		u16 clut = 0, tpage = 0;
		int w = i + 1;
		for (int v = 0; v < n; v++)
		{
			u32 c = (v == 0 || !gouraud) ? cmd : words[w++];
			col[v][0] = raw ? 0x80 : (u8)c;
			col[v][1] = raw ? 0x80 : (u8)(c >> 8);
			col[v][2] = raw ? 0x80 : (u8)(c >> 16);
			col[v][3] = 0xff;

			u32 xy = words[w++];
			const float *view = Rtx_CapLookup(xy, serial);
			if (view == NULL)
			{
				goto fail;
			}
			Rtx_ViewToWorld(view, pos[v]);

			if (textured)
			{
				u32 t = words[w++];
				uv[v][0] = (u8)t;
				uv[v][1] = (u8)(t >> 8);
				if (v == 0)
				{
					clut = (u16)(t >> 16);
				}
				else if (v == 1)
				{
					tpage = (u16)(t >> 16);
				}
			}
			else
			{
				uv[v][0] = uv[v][1] = 0;
			}
		}

		static const int triIdx[2][3] = {{0, 1, 2}, {1, 3, 2}};
		for (int t = 0; t < (quad ? 2 : 1); t++)
		{
			if (s_rtx.numDyn >= (int)RTX_MAX_DYN_TRIS)
			{
				goto fail;
			}
			struct RtxDynTri *d = &Rtx_DynTris()[s_rtx.numDyn++];
			for (int c = 0; c < 3; c++)
			{
				int v = triIdx[t][c];
				memcpy(d->p[c], pos[v], sizeof(d->p[c]));
				d->uv[c][0] = uv[v][0];
				d->uv[c][1] = uv[v][1];
				memcpy(d->col[c], col[v], 4);
			}
			d->clut = clut;
			d->tpage = tpage;
			d->flags = textured ? RTX_TRI_TEXTURED : 0;
			d->object = serial;
		}
		polys++;
		i += size;
	}
	return polys > 0;

fail:
	s_rtx.numDyn = startDyn;
	return 0;
}

// ---- emitters: glowing (additive) things become point lights ---------------

static u16 Rtx_Vram16(int x, int y)
{
	return NativeRenderer_RtxVRAM()[(y & 511) * 1024 + (x & 1023)];
}

// PS1 texel at (u, v) like the GPU reads it; returns 0 for colour key 0x0000
static int Rtx_Texel(u16 tpage, u16 clut, int u, int v, float *rgb)
{
	int mode = (tpage >> 7) & 3, bx = (tpage & 15) * 64, by = ((tpage >> 4) & 1) * 256;
	int cx = (clut & 63) * 16, cy = (clut >> 6) & 511;
	u &= 255;
	v &= 255;
	u16 c;
	if (mode == 0)
	{
		c = Rtx_Vram16(cx + ((Rtx_Vram16(bx + (u >> 2), by + v) >> ((u & 3) * 4)) & 15), cy);
	}
	else if (mode == 1)
	{
		c = Rtx_Vram16(cx + ((Rtx_Vram16(bx + (u >> 1), by + v) >> ((u & 1) * 8)) & 255), cy);
	}
	else
	{
		c = Rtx_Vram16(bx + u, by + v);
	}
	if (c == 0)
	{
		return 0;
	}
	rgb[0] = (c & 31) / 31.0f;
	rgb[1] = ((c >> 5) & 31) / 31.0f;
	rgb[2] = ((c >> 10) & 31) / 31.0f;
	return 1;
}

// average opaque texel colour over a few points of a texture rectangle
static int Rtx_AverageTexels(u16 tpage, u16 clut, const u8 (*uv)[2], int n, float *avg)
{
	int umin = 255, umax = 0, vmin = 255, vmax = 0;
	for (int i = 0; i < n; i++)
	{
		umin = uv[i][0] < umin ? uv[i][0] : umin;
		umax = uv[i][0] > umax ? uv[i][0] : umax;
		vmin = uv[i][1] < vmin ? uv[i][1] : vmin;
		vmax = uv[i][1] > vmax ? uv[i][1] : vmax;
	}
	int count = 0;
	avg[0] = avg[1] = avg[2] = 0.0f;
	for (int sy = 0; sy < 5; sy++)
	{
		for (int sx = 0; sx < 5; sx++)
		{
			float rgb[3];
			int u = umin + ((umax - umin) * (2 * sx + 1)) / 10, v = vmin + ((vmax - vmin) * (2 * sy + 1)) / 10;
			if (Rtx_Texel(tpage, clut, u, v, rgb))
			{
				avg[0] += rgb[0];
				avg[1] += rgb[1];
				avg[2] += rgb[2];
				count++;
			}
		}
	}
	if (count == 0)
	{
		return 0;
	}
	avg[0] /= count;
	avg[1] /= count;
	avg[2] /= count;
	return 1;
}

static void Rtx_AddEmitter(const float *pos, const float *color, float weight, float radius)
{
	float lum = 0.299f * color[0] + 0.587f * color[1] + 0.114f * color[2];
	if ((lum < 0.05f) || (s_rtx.numEmitters >= RTX_MAX_EMITTERS))
	{
		return;
	}
	struct RtxEmitter *e = &s_rtx.emitters[s_rtx.numEmitters++];
	for (int i = 0; i < 3; i++)
	{
		e->pos[i] = pos[i];
		e->color[i] = color[i] * weight;
	}
	e->weight = weight;
	e->radius = radius;
}

// additive-blended polygons of a model packet (turbo flames, fire, glows)
static void Rtx_EmitterPacket(u32 *words, int numWords, u16 serial, int anyBlend)
{
	int i = 0;
	int abr = 0; // draw mode blend (DR_MODE / E1) for untextured polygons

	while (i < numWords)
	{
		u32 cmd = words[i];
		u32 code = cmd >> 24;
		if (code == 0xe1)
		{
			abr = (cmd >> 5) & 3;
			i++;
			continue;
		}
		if ((code < 0x20) || (code >= 0x40))
		{
			if ((code == 0) || (code > 0xe0))
			{
				i++;
				continue;
			}
			return;
		}
		int gouraud = (code & 0x10) != 0, quad = (code & 0x08) != 0, textured = (code & 0x04) != 0, semi = (code & 0x02) != 0;
		int n = quad ? 4 : 3;
		int size = 1 + n * (1 + textured) + (gouraud ? n - 1 : 0);
		if (i + size > numWords)
		{
			return;
		}

		float pos[4][3], col[3] = {0, 0, 0};
		u8 uv[4][2];
		u16 clut = 0, tpage = 0;
		int w = i + 1, found = 1;
		for (int v = 0; v < n; v++)
		{
			u32 c = (v == 0 || !gouraud) ? cmd : words[w++];
			col[0] += (u8)c / 128.0f / n;
			col[1] += (u8)(c >> 8) / 128.0f / n;
			col[2] += (u8)(c >> 16) / 128.0f / n;
			const float *view = Rtx_CapLookup(words[w++], serial);
			if (view != NULL)
			{
				Rtx_ViewToWorld(view, pos[v]);
			}
			else
			{
				found = 0;
			}
			if (textured)
			{
				u32 t = words[w++];
				uv[v][0] = (u8)t;
				uv[v][1] = (u8)(t >> 8);
				if (v == 0)
				{
					clut = (u16)(t >> 16);
				}
				else if (v == 1)
				{
					tpage = (u16)(t >> 16);
				}
			}
		}
		int blend = textured ? ((tpage >> 5) & 3) : abr;
		if ((anyBlend || (semi && (blend == 1))) && found)
		{
			float tex[3] = {1, 1, 1};
			if (!textured || Rtx_AverageTexels(tpage, clut, uv, n, tex))
			{
				float c[3] = {col[0] * tex[0], col[1] * tex[1], col[2] * tex[2]};
				float centre[3] = {0, 0, 0}, radius = 0.0f;
				for (int v = 0; v < n; v++)
				{
					for (int k = 0; k < 3; k++)
					{
						centre[k] += pos[v][k] / n;
					}
				}
				for (int v = 0; v < n; v++)
				{
					float dx = pos[v][0] - centre[0], dy = pos[v][1] - centre[1], dz = pos[v][2] - centre[2];
					float d = sqrtf(dx * dx + dy * dy + dz * dz);
					radius = d > radius ? d : radius;
				}
				Rtx_AddEmitter(centre, c, 0.5f + radius * radius / 1500.0f, radius); // bigger flame, more light
			}
		}
		i += size;
	}
}

void Rtx_OnParticle(struct PushBuffer *pb, s32 viewX4, s32 viewY4, s32 viewZ4, u32 color, const struct Icon *icon)
{
	if ((RTX_HDR == NULL) || !s_rtx.enabled || (pb != &sdata->gGT->pushBuffer[0]) || (icon == NULL))
	{
		return;
	}
	const struct TextureLayout *t = &icon->texLayout;
	// fire, sparks, glows: additive sprites, or bright coloured ones (the colour
	// word carries no reliable semi-transparency bit here); grey smoke is skipped
	int additive = ((t->tpage >> 5) & 3) == 1;
	Rtx_CaptureCamera(pb);
	u8 uv[4][2] = {{t->u0, t->v0}, {t->u1, t->v1}, {t->u2, t->v2}, {t->u3, t->v3}};
	float tex[3];
	if (!Rtx_AverageTexels(t->tpage, t->clut, uv, 4, tex))
	{
		return;
	}
	float view[3] = {viewX4 * 0.25f, viewY4 * 0.25f, viewZ4 * 0.25f}, pos[3];
	Rtx_ViewToWorld(view, pos);
	float c[3] = {(u8)color / 128.0f * tex[0], (u8)(color >> 8) / 128.0f * tex[1], (u8)(color >> 16) / 128.0f * tex[2]};
	if (!additive)
	{
		float mx = fmaxf(c[0], fmaxf(c[1], c[2])), mn = fminf(c[0], fminf(c[1], c[2]));
		if ((mx < 0.3f) || ((mx - mn) < 0.35f * mx))
		{
			return; // dim or grey (smoke, dust)
		}
	}
	Rtx_AddEmitter(pos, c, 1.0f, 40.0f);
}

// greedy clustering of this frame's emitters into at most RTX_MAX_DYN_LIGHTS lights
static void Rtx_BuildDynLights(void)
{
	const float mergeDist = 260.0f;
	struct RtxEmitter clusters[RTX_MAX_DYN_LIGHTS * 4];
	int numClusters = 0;

	for (int i = 0; i < s_rtx.numEmitters; i++)
	{
		const struct RtxEmitter *e = &s_rtx.emitters[i];
		int best = -1;
		for (int c = 0; c < numClusters; c++)
		{
			const struct RtxEmitter *k = &clusters[c];
			float dx = k->pos[0] / k->weight - e->pos[0], dy = k->pos[1] / k->weight - e->pos[1], dz = k->pos[2] / k->weight - e->pos[2];
			if (dx * dx + dy * dy + dz * dz < mergeDist * mergeDist)
			{
				best = c;
				break;
			}
		}
		if (best < 0)
		{
			if (numClusters >= (int)(sizeof(clusters) / sizeof(clusters[0])))
			{
				continue;
			}
			best = numClusters++;
			memset(&clusters[best], 0, sizeof(clusters[best]));
		}
		struct RtxEmitter *k = &clusters[best];
		for (int j = 0; j < 3; j++)
		{
			k->pos[j] += e->pos[j] * e->weight;
			k->color[j] += e->color[j];
		}
		k->weight += e->weight;
		k->radius = e->radius > k->radius ? e->radius : k->radius;
	}

	// strongest first
	for (int a = 0; a < numClusters; a++)
	{
		for (int b = a + 1; b < numClusters; b++)
		{
			if (clusters[b].weight > clusters[a].weight)
			{
				struct RtxEmitter t = clusters[a];
				clusters[a] = clusters[b];
				clusters[b] = t;
			}
		}
	}

	int n = numClusters < (int)RTX_MAX_DYN_LIGHTS ? numClusters : (int)RTX_MAX_DYN_LIGHTS;
	for (int c = 0; c < n; c++)
	{
		const struct RtxEmitter *k = &clusters[c];
		struct RtxPointLight *l = &RTX_HDR->dynLights[c];
		// colour = average colour of what glows; more glowing stuff = brighter
		float strength = 2.0f * sqrtf(k->weight);
		strength = strength > 14.0f ? 14.0f : strength;
		for (int j = 0; j < 3; j++)
		{
			l->pos[j] = k->pos[j] / k->weight;
			l->color[j] = k->color[j] / k->weight * strength;
		}
		l->height = 0.0f;
		l->falloff = 300.0f + 4.0f * k->radius;
		l->size = 20.0f + 0.5f * k->radius;
	}
	RTX_HDR->numDynLights = (u32)n;
}

int Rtx_ClassifyPacket(u32 *packet, int lengthWords)
{
	const struct RtxRange *r = Rtx_FindRange((u32)packet);
	if (r == NULL)
	{
		return RTX_PACKET_DRAW;
	}
	switch (r->tag)
	{
	case RTX_TAG_LEVEL:
	case RTX_TAG_SHADOW:
		return RTX_PACKET_SKIP;
	case RTX_TAG_SKY:
		return RTX_PACKET_SKY;
	case RTX_TAG_INSTANCE:
		(void)lengthWords;
		return Rtx_IsCaptured((u32)packet) ? RTX_PACKET_SKIP : RTX_PACKET_DRAW;
	default:
		return RTX_PACKET_DRAW;
	}
}

int Rtx_SkyPhase(void)
{
	return s_rtx.skyPhase;
}

void Rtx_EndSkyPhase(void)
{
	s_rtx.skyPhase = 0;
	NativeRenderer_RtxSkySnapshot();
	int w, h;
	const u8 *image = NULL;
	NativeRenderer_RtxViewportSize(&w, &h);
	NativeRenderer_RtxSetImage(Rtx_RenderFrame(w, h, &image) ? image : NULL, w, h);
}

int Rtx_RenderFrame(int outW, int outH, const u8 **rgba)
{
	if (!Rtx_FrameActive())
	{
		return 0;
	}
	if (outW > (int)RTX_MAX_W)
	{
		outW = RTX_MAX_W;
	}
	if (outH > (int)RTX_MAX_H)
	{
		outH = RTX_MAX_H;
	}

	Rtx_RefreshTexSlots();
	memcpy(s_rtx.base + RTX_OFF_VRAM, NativeRenderer_RtxVRAM(), RTX_VRAM_W * RTX_VRAM_H * 2);
	Rtx_BuildDynLights();
	if (SDL_getenv("CTR_RTX_DEBUG") && (RTX_HDR->frame % 300 == 0))
	{
		float cam[3];
		const float zero[3] = {0, 0, 0};
		Rtx_ViewToWorld(zero, cam);
		printf("[RTX] frame %u: %d dyn tris, %d emitters -> %u lights, %d ranges, cam %.0f %.0f %.0f, renderer %.2f ms, %u spp\n", RTX_HDR->frame, s_rtx.numDyn, s_rtx.numEmitters, RTX_HDR->numDynLights, s_rtx.numRanges, cam[0], cam[1],
		       cam[2], RTX_HDR->renderMs, RTX_HDR->traceSpp);
		for (int i = 0; i < s_rtx.numDyn && i < 3; i++)
		{
			struct RtxDynTri *d = &Rtx_DynTris()[i];
			printf("[RTX]   tri %d obj %d: %.0f %.0f %.0f / %.0f %.0f %.0f / %.0f %.0f %.0f\n", i, d->object, d->p[0][0], d->p[0][1], d->p[0][2], d->p[1][0],
			       d->p[1][1], d->p[1][2], d->p[2][0], d->p[2][1], d->p[2][2]);
		}
	}
	RTX_HDR->numDynTris = (u32)s_rtx.numDyn;
	RTX_HDR->outW = (u32)outW;
	RTX_HDR->outH = (u32)outH;
	RTX_HDR->dispW = 512.0f;
	RTX_HDR->dispH = 216.0f;
	RTX_HDR->debugView = (u32)s_rtx.debugView;
	RTX_HDR->lightMode = s_rtx.lightMode ? 0u : 1u; // default full relight, F5 hybrid
	RTX_HDR->frame++;

	__sync_synchronize();
	u32 req = RTX_HDR->frameReq + 1;
	RTX_HDR->frameReq = req;

	u64 start = SDL_GetTicksNS();
	while (RTX_HDR->frameDone != req)
	{
		if (SDL_GetTicksNS() - start > 250000000ull)
		{
			static int warned;
			if (!warned++)
			{
				printf("[RTX] renderer timed out, frame drawn without ray tracing\n");
			}
			return 0;
		}
		if (kill(s_rtx.rendererPid, 0) != 0)
		{
			printf("[RTX] renderer exited, ray tracing off\n");
			RTX_HDR->rendererReady = 0;
			return 0;
		}
		sched_yield();
	}
	__sync_synchronize();
	*rgba = s_rtx.base + RTX_OFF_IMAGE;
	return 1;
}
