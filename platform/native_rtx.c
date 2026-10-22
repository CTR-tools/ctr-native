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
#include <linux/futex.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

extern char **environ;
// not declared under -std=c99 without _POSIX_C_SOURCE (unity build includes headers first)
int ftruncate(int fd, off_t length);
int kill(pid_t pid, int sig);
long syscall(long number, ...);

// sleep while *addr == val (at most ns), wake the renderer's sleeper (rtx_shared.h)
static void Rtx_FutexWait(volatile u32 *addr, u32 val, long ns)
{
	struct
	{
		long sec, nsec; // (the kernel's timespec of this 32-bit futex call)
	} ts = {0, ns};
	syscall(SYS_futex, (u32 *)addr, FUTEX_WAIT, val, &ts, NULL, 0);
}

static void Rtx_FutexWake(volatile u32 *addr)
{
	syscall(SYS_futex, (u32 *)addr, FUTEX_WAKE, 0x7fffffff, NULL, NULL, 0);
}

const u16 *NativeRenderer_RtxVRAM(void);

int g_rtxCapture;

#define RTX_MAX_RANGES  16384
#define RTX_CAP_BITS    18
#define RTX_LEVEL_SERIAL 0xffff // projections of the level (instances count 1..0xfffe)
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
	float z;      // RTX_TAG_DEPTH: view z of the whole range
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
	struct RtxSettings settings; // F12 overlay keys 1-8, CTR_RTX_* env
	// last request (pipelined: its image is the one shown next)
	int prevPipelined, prevW, prevH;
	u32 prevGen;

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
	// glows that stay put (torches, braziers, statue fire) are remembered where
	// they were seen: the game only draws them while they are on screen, but
	// their light reaches past it (Rtx_BuildDynLights)
	struct RtxRememberedLight
	{
		float pos[3], color[3]; // smoothed, as RtxPointLight
		float anchor[3];        // where it was first seen
		float falloff, size;
		u32 lastGen;  // game frame last seen
		u32 levelGen; // level it belongs to
		u16 seen;     // frames seen in the same place
		u8 moving;    // moved since first seen (kart flames): never remembered
		u8 matched;   // seen this frame
	} remembered[128];
	int worldCapture; // record GTE projections outside tags (skidmarks, effects) for the overlay depth test
	int numLevelCap;  // level polygons of this frame's OT walk (the renderer gets them next frame)
	u16 capSerial;    // key of the projections being recorded: instance serial, 0 = world
	u32 otBase;       // pushBuffer[0] ordering table (0x400 buckets) and the UI one, this frame
	u32 uiOtStart, uiOtEnd;

	// GL overlay depth: context of the packet being parsed
	int ctxActive;
	int ctxWorld;  // last OT entry walked was pushBuffer[0]'s
	int ctxBucket; // its index
	u16 ctxSerial;
	float ctxZ;
	// bucket -> view z, fitted from model packets (OT index ~ z / 64)
	float zPerBucket;
	double calZ, calB;
	int calN;

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

	// packets captured into dynamic triangles this frame: word offsets (after the
	// tag) of the polygon commands GL skips, RTX_MASK_ALL = the whole packet
	u32 capturedKey[1 << 15];
	u32 capturedGen[1 << 15];
	u32 capturedMask[1 << 15];
	u32 ctxMask; // of the packet being parsed

	// smoke particles of this frame's OT walk as camera-facing quads (shadows, next frame)
	struct RtxDynTri smoke[2048];
	int numSmoke;
	float partOfx, partOfy, partH; // the GTE projection particles are drawn with

	// karts this frame: the dynamic triangles of each driver's body, so its
	// wheels' range can move it onto the traced floor with them (Rtx_EndTag)
	struct
	{
		const void *inst;
		int start, end;
	} kartBody[16];
	int numKartBody;
	const void *curKart;   // instance being captured is a driver's body
	const void *curWheels; // wheel range being captured: of this driver instance
	struct
	{
		const void *inst;
		float dy;
		u32 gen;
	} kartLift[16]; // smoothed correction per driver
	double statGapSum, statGapAbs;
	int statGapN, statGapFar, statGapBody, statCapAnimated;

	// textures that animate (boosters): tpage, clut, uv rectangle of every frame,
	// to tell their polygons among the level as drawn (RTX_TRI_ANIMATED)
	struct
	{
		u16 tpage, clut;
		u8 u0, v0, u1, v1;
	} animRect[1024];
	int numAnimRect;

	// capture statistics (CTR_RTX_DEBUG), model packets this frame
	int statCaptured, statNoProj, statSemi, statInstances;
	// emitter statistics (CTR_RTX_DEBUG): instances by mode, candidates by source / fate
	// (summed over the 300 frames between reports)
	int statInstMode[3], statEmitGlow, statEmitSemi, statEmitOpaque, statEmitParticle, statEmitGrey, statEmitDim, statEmitNoProj;
	int statPartAdditive, statPartVivid, statPartWhite, statPartDull, statPartNoTex;
	int statPolyNoTex, statPolyNotGlow, statEmitAdded, statEmitFull, statLights;
	int statMemMoved, statMemGone, statMemOld;
} s_rtx;

#define RTX_MASK_ALL 0xffffffffu

#define RTX_HDR (s_rtx.hdr)

static struct RtxLevelVert *Rtx_LevelVerts(void) { return (struct RtxLevelVert *)(s_rtx.base + RTX_OFF_LEVEL_VERTS); }
static struct RtxLevelTri *Rtx_LevelTris(void) { return (struct RtxLevelTri *)(s_rtx.base + RTX_OFF_LEVEL_TRIS); }
static struct RtxTexSlot *Rtx_TexSlots(void) { return (struct RtxTexSlot *)(s_rtx.base + RTX_OFF_TEX_SLOTS); }
static struct RtxDynTri *Rtx_DynTris(void) { return (struct RtxDynTri *)(s_rtx.base + RTX_OFF_DYN_TRIS); }

static void Rtx_InitSettings(void);

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
	const char *view = SDL_getenv("CTR_RTX_VIEW"); // start in a debug view (as F6): 1 albedo, 2 normals, 3 direct, 4 indirect, 5 texture layer misses
	s_rtx.debugView = view ? (atoi(view) % 8) : 0;
	const char *mode = SDL_getenv("CTR_RTX_HYBRID"); // start in hybrid lighting (as F5)
	s_rtx.lightMode = (mode && mode[0] == '1');
	Rtx_InitSettings();
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

static int Rtx_EnvInt(const char *name, int fallback)
{
	const char *v = SDL_getenv(name);
	return v ? atoi(v) : fallback;
}

// renderer settings: defaults (the user's preferred set, native/preferred-defaults.png),
// CTR_RTX_* env vars override (F12 overlay changes them live)
static void Rtx_InitSettings(void)
{
	struct RtxSettings *S = &s_rtx.settings;
	S->renderScale = (u32)Rtx_EnvInt("CTR_RTX_SCALE", 67);
	S->renderScale = S->renderScale < 25 ? 25 : (S->renderScale > 100 ? 100 : S->renderScale);
	S->quality = (u32)Rtx_EnvInt("CTR_RTX_QUALITY", 1) & 3u;
	const char *budget = SDL_getenv("CTR_RTX_BUDGET_MS");
	S->budgetMs = budget ? (float)atof(budget) : 28.0f;
	S->taa = Rtx_EnvInt("CTR_RTX_TAA", 1) != 0;
	S->texFilter = Rtx_EnvInt("CTR_RTX_TEXFILTER", 1) != 0;
	S->specular = Rtx_EnvInt("CTR_RTX_SPEC", 1) != 0;
	S->fog = Rtx_EnvInt("CTR_RTX_FOG", 0) != 0;
	S->pipelined = Rtx_EnvInt("CTR_RTX_PIPELINE", 1) != 0;
}

static const char *s_rtxViewNames[] = {"final", "albedo", "normal", "direct", "indirect", "texture layer misses (red)", "sun x shadow", "sun, smooth normals"};
static const char *s_rtxQualityNames[] = {"low (2 spp)", "medium (4 spp)", "high (8 spp)", "ultra (16 spp)"};

const char *Rtx_KeyStatus(int scancode)
{
	if (s_rtx.hdr == NULL)
	{
		return "unavailable";
	}
	switch (scancode)
	{
	case SDL_SCANCODE_F5:
		return s_rtx.lightMode ? "hybrid" : "full relight";
	case SDL_SCANCODE_F6:
		return s_rtxViewNames[s_rtx.debugView & 7];
	case SDL_SCANCODE_F7:
		return s_rtx.enabled ? (RTX_HDR->rendererReady ? "on" : "on (renderer not ready)") : "off";
	}
	return "";
}

// overlay: the renderer settings (keys 1-8) and its timings
int Rtx_OverlayText(char *buf, int size)
{
	if (s_rtx.hdr == NULL)
	{
		return snprintf(buf, size, "ray tracing unavailable (renderer not started)\n");
	}
	const struct RtxSettings *S = &s_rtx.settings;
	const float *ms = RTX_HDR->passMs;
	return snprintf(buf, size,
	                "-- ray tracing: keys 1-8 change, Shift+key back --\n"
	                "1 render scale    %u%% (%ux%u -> %ux%u)\n"
	                "2 quality         %s, now %u spp\n"
	                "3 TAA             %s\n"
	                "4 texture filter  %s\n"
	                "5 specular        %s\n"
	                "6 fog / shafts    %s\n"
	                "7 pipelined       %s\n"
	                "8 trace budget    %.0f ms\n"
	                "-- renderer --\n"
	                "GPU %5.2f ms  CPU %5.2f ms  frame %5.2f ms\n"
	                " build %.2f  trace %.2f  cache %.2f\n"
	                " a-trous %.2f  final %.2f  copy %.2f\n",
	                S->renderScale, RTX_HDR->inW, RTX_HDR->inH, RTX_HDR->outW, RTX_HDR->outH, s_rtxQualityNames[S->quality & 3], RTX_HDR->traceSpp,
	                S->taa ? "on" : "off", S->texFilter ? "bilinear" : "nearest (PS1)", S->specular ? "on" : "off", S->fog ? "on" : "off",
	                S->pipelined ? "on (+1 frame latency)" : "off", S->budgetMs, ms[RTX_PASS_GPU], ms[RTX_PASS_CPU], RTX_HDR->renderMs,
	                ms[RTX_PASS_BUILD], ms[RTX_PASS_TRACE], ms[RTX_PASS_CACHE], ms[RTX_PASS_ATROUS], ms[RTX_PASS_FINAL], ms[RTX_PASS_COPY]);
}

// overlay key 1-8: step that setting (back: the other way)
void Rtx_OverlayKey(int digit, int back)
{
	struct RtxSettings *S = &s_rtx.settings;
	int dir = back ? -1 : 1;
	static const u32 scales[] = {50, 67, 75, 100};
	static const float budgets[] = {8.0f, 10.0f, 14.0f, 18.0f, 22.0f, 28.0f};
	switch (digit)
	{
	case 1:
	{
		int i = 0;
		while ((i < 3) && (scales[i] < S->renderScale))
		{
			i++;
		}
		i = (scales[i] == S->renderScale) ? (i + dir + 4) % 4 : (back ? (i + 3) % 4 : i);
		S->renderScale = scales[i];
		break;
	}
	case 2:
		S->quality = (S->quality + (u32)(dir + 4)) & 3u;
		break;
	case 3:
		S->taa ^= 1;
		break;
	case 4:
		S->texFilter ^= 1;
		break;
	case 5:
		S->specular ^= 1;
		break;
	case 6:
		S->fog ^= 1;
		break;
	case 7:
		S->pipelined ^= 1;
		break;
	case 8:
	{
		int i = 0;
		while ((i < 5) && (budgets[i] < S->budgetMs - 0.5f))
		{
			i++;
		}
		S->budgetMs = budgets[(i + dir + 6) % 6];
		break;
	}
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
		s_rtx.debugView = (s_rtx.debugView + 1) % 6;
		printf("[RTX] debug view: %s\n", s_rtxViewNames[s_rtx.debugView]);
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
	s_rtx.numKartBody = 0;
	s_rtx.curKart = s_rtx.curWheels = NULL;
	s_rtx.numEmitters = 0;
	s_rtx.camForCapture = 0;
	if (s_rtx.calN >= 16)
	{
		float k = (float)(s_rtx.calZ / s_rtx.calB);
		s_rtx.zPerBucket = k < 8.0f ? 8.0f : (k > 1024.0f ? 1024.0f : k);
	}
	else if (s_rtx.zPerBucket <= 0.0f)
	{
		s_rtx.zPerBucket = 64.0f;
	}
	s_rtx.calZ = s_rtx.calB = 0.0;
	s_rtx.calN = 0;
	s_rtx.worldCapture = (RTX_HDR != NULL) && s_rtx.enabled && RTX_HDR->rendererReady;
	s_rtx.statCaptured = s_rtx.statNoProj = s_rtx.statSemi = s_rtx.statInstances = 0;
	s_rtx.capSerial = 0;
	s_rtx.instScale = 1.0f;
	g_rtxCapture = s_rtx.worldCapture;
}

// which projections the GTE hook records, from the innermost open tag
static void Rtx_UpdateCapture(void)
{
	int top = (s_rtx.tagDepth > 0) ? s_rtx.tagStack[s_rtx.tagDepth - 1] : -1;
	if (top == RTX_TAG_INSTANCE)
	{
		s_rtx.capSerial = s_rtx.serial;
		g_rtxCapture = 1;
	}
	else if (top < 0)
	{
		s_rtx.capSerial = 0;
		s_rtx.instScale = 1.0f;
		g_rtxCapture = s_rtx.worldCapture;
	}
	else if (top == RTX_TAG_LEVEL)
	{
		// the level as the PS1 draws it (LOD, hi-detail ground, animated textures):
		// its polygons are decoded in the OT walk (Rtx_ClassifyPacket)
		s_rtx.capSerial = RTX_LEVEL_SERIAL;
		s_rtx.instScale = 1.0f;
		g_rtxCapture = s_rtx.worldCapture;
	}
	else
	{
		g_rtxCapture = 0; // level, sky, shadows, HUD models
	}
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
		if ((s_rtx.serial == 0) || (s_rtx.serial == RTX_LEVEL_SERIAL))
		{
			s_rtx.serial = 1;
		}
	}
	Rtx_UpdateCapture();
}

static u32 Rtx_CapturePacketTo(u32 *words, int numWords, u16 serial, int allowSemi, struct RtxDynTri *out, int *num, int max);
static void Rtx_SeatKart(const void *inst, int wheelStart, int wheelEnd);
#define Rtx_CapturePacket(words, numWords, serial, allowSemi) \
	Rtx_CapturePacketTo(words, numWords, serial, allowSemi, Rtx_DynTris(), &s_rtx.numDyn, (int)RTX_MAX_DYN_TRIS)
static void Rtx_EmitterPacket(u32 *words, int numWords, u16 serial, int anyBlend);
static void Rtx_MarkCaptured(u32 packet, u32 mask);

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
	Rtx_UpdateCapture();

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
	r->z = 0.0f;
	s_rtx.allowSemi = 0;
	s_rtx.instMode = RTX_INST_CAPTURE;

	// model packets are allocated back to back: decode them now, so the ray
	// traced image can be rendered before the GL overlay pass draws over it
	int dynStart = s_rtx.numDyn;
	const void *kart = s_rtx.curKart, *wheelsOf = s_rtx.curWheels;
	s_rtx.curKart = s_rtx.curWheels = NULL;
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
			else
			{
				// opaque polygons are traced; the rest (glows, lamp cones) stays GL drawn and lights the scene
				u32 mask = Rtx_CapturePacket(p + 1, len, r->serial, r->allowSemi);
				if (mask != 0)
				{
					Rtx_MarkCaptured((u32)p, mask);
				}
				if (mask != RTX_MASK_ALL)
				{
					Rtx_EmitterPacket(p + 1, len, r->serial, 0);
				}
			}
			p += 1 + len;
		}
		if ((kart != NULL) && (s_rtx.numKartBody < 16))
		{
			s_rtx.kartBody[s_rtx.numKartBody].inst = kart;
			s_rtx.kartBody[s_rtx.numKartBody].start = dynStart;
			s_rtx.kartBody[s_rtx.numKartBody].end = s_rtx.numDyn;
			s_rtx.numKartBody++;
		}
		if (wheelsOf != NULL)
		{
			Rtx_SeatKart(wheelsOf, dynStart, s_rtx.numDyn);
		}
	}
}

static u32 Rtx_PtrHash(u32 p)
{
	u32 h = p * 0x9E3779B1u;
	return (h ^ (h >> 16)) & ((1 << 15) - 1);
}

static void Rtx_MarkCaptured(u32 packet, u32 mask)
{
	u32 h = Rtx_PtrHash(packet);
	for (int probe = 0; probe < 64; probe++)
	{
		u32 i = (h + probe) & ((1 << 15) - 1);
		if (s_rtx.capturedGen[i] != s_rtx.gen)
		{
			s_rtx.capturedGen[i] = s_rtx.gen;
			s_rtx.capturedKey[i] = packet;
			s_rtx.capturedMask[i] = mask;
			return;
		}
	}
}

static u32 Rtx_CapturedMask(u32 packet)
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
			return s_rtx.capturedMask[i];
		}
	}
	return 0;
}

static int Rtx_PolyWords(u32 cmd)
{
	u32 code = cmd >> 24;
	int n = (code & 0x08) ? 4 : 3;
	return 1 + n * (1 + ((code & 0x04) != 0)) + ((code & 0x10) ? n - 1 : 0);
}

// GL parse of a partly captured packet: words to skip at cmd (0 = draw it)
int Rtx_SkipCommand(u32 *packet, u32 *cmd)
{
	if (s_rtx.ctxMask == 0)
	{
		return 0;
	}
	u32 offset = (u32)(cmd - (packet + 1));
	if ((offset >= 32) || !(s_rtx.ctxMask & (1u << offset)))
	{
		return 0;
	}
	return Rtx_PolyWords(*cmd);
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
	s_rtx.statInstMode[s_rtx.instMode]++;

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
	s_rtx.statInstances++;
	s_rtx.curKart = ((inst->thread != NULL) && ((inst->thread->modelIndex == DYNAMIC_PLAYER) || (inst->thread->modelIndex == DYNAMIC_ROBOT_CAR))) ? inst : NULL;
	Rtx_BeginTag(RTX_TAG_INSTANCE, primMem);
}

void Rtx_BeginScaled(struct PrimMem *primMem, struct PushBuffer *pb, float viewScale, const struct Instance *wheelsOf)
{
	if ((RTX_HDR == NULL) || (pb != &sdata->gGT->pushBuffer[0]))
	{
		Rtx_BeginTag(RTX_TAG_NONE, primMem);
		return;
	}
	s_rtx.curWheels = wheelsOf;
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

// Screen XY -> view position of what was projected there. Several vertices of
// one model can land on the same pixel (front and back of a character, a
// distant seal): each is kept, and polygons pick the set that fits (Rtx_CapLookupAll).
#define RTX_CAP_PROBES 32
#define RTX_CAP_CANDIDATES 4

void Rtx_OnProject(u32 sxy, s32 mac1, s32 mac2, s32 mac3, int sf)
{
	u32 h = Rtx_CapHash(sxy, s_rtx.capSerial);
	float scale = (sf ? 1.0f : (1.0f / 4096.0f)) * s_rtx.instScale;
	float v[3] = {mac1 * scale, mac2 * scale, mac3 * scale};

	for (int probe = 0; probe < RTX_CAP_PROBES; probe++)
	{
		struct RtxCapEntry *e = &s_rtx.cap[(h + probe) & (RTX_CAP_SIZE - 1)];
		if (e->gen != s_rtx.gen)
		{
			e->gen = s_rtx.gen;
			e->sxy = sxy;
			e->serial = s_rtx.capSerial;
			memcpy(e->v, v, sizeof(v));
			return;
		}
		if ((e->sxy == sxy) && (e->serial == s_rtx.capSerial) && (fabsf(e->v[0] - v[0]) + fabsf(e->v[1] - v[1]) + fabsf(e->v[2] - v[2]) < 1.0f))
		{
			return; // the same vertex again
		}
	}
}

// every view position projected to sxy (up to RTX_CAP_CANDIDATES), oldest first
static int Rtx_CapLookupAll(u32 sxy, u32 serial, const float **out)
{
	u32 h = Rtx_CapHash(sxy, serial);
	int n = 0;

	for (int probe = 0; (probe < RTX_CAP_PROBES) && (n < RTX_CAP_CANDIDATES); probe++)
	{
		struct RtxCapEntry *e = &s_rtx.cap[(h + probe) & (RTX_CAP_SIZE - 1)];
		if (e->gen != s_rtx.gen)
		{
			break;
		}
		if ((e->sxy == sxy) && (e->serial == serial))
		{
			out[n++] = e->v;
		}
	}
	return n;
}

static const float *Rtx_CapLookup(u32 sxy, u32 serial)
{
	const float *c[RTX_CAP_CANDIDATES];
	return Rtx_CapLookupAll(sxy, serial, c) ? c[0] : NULL;
}

// a polygon's vertices among their candidates: the set closest together in depth
// (a polygon's corners are neighbours on the model). 0 when a vertex wasn't projected
static int Rtx_CapPolygon(const u32 *xy, int n, u32 serial, const float **out)
{
	const float *cand[4][RTX_CAP_CANDIDATES];
	int count[4], combos = 1;
	for (int v = 0; v < n; v++)
	{
		count[v] = Rtx_CapLookupAll(xy[v], serial, cand[v]);
		if (count[v] == 0)
		{
			return 0;
		}
		combos *= count[v];
	}
	float best = 1e30f;
	for (int c = 0; c < combos; c++)
	{
		int k = c;
		const float *pick[4];
		float zmin = 1e30f, zmax = -1e30f;
		for (int v = 0; v < n; v++)
		{
			pick[v] = cand[v][k % count[v]];
			k /= count[v];
			zmin = fminf(zmin, pick[v][2]);
			zmax = fmaxf(zmax, pick[v][2]);
		}
		if (zmax - zmin < best)
		{
			best = zmax - zmin;
			memcpy(out, pick, sizeof(pick[0]) * n);
		}
	}
	return 1;
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

// the level's sky colours by direction (8 azimuths x 8 elevations, top row up),
// from its skybox mesh vertex colours, else its clear colours
static void Rtx_UploadSky(struct Level *level)
{
	float sum[RTX_SKY_H][RTX_SKY_W][4];
	memset(sum, 0, sizeof(sum));
	const struct Skybox *sky = level->ptr_skybox;
	int n = 0;
	if ((sky != NULL) && (sky->ptrVertex != NULL) && (sky->numVertex > 0) && (sky->numVertex < 65536))
	{
		for (int i = 0; i < sky->numVertex; i++)
		{
			const struct ShortVertex *v = &sky->ptrVertex[i];
			float x = v->Position.vx, y = v->Position.vy, z = v->Position.vz;
			float len = sqrtf(x * x + y * y + z * z);
			if (len < 1.0f)
			{
				continue;
			}
			// CTR world is Y up
			float el = asinf(fmaxf(-1.0f, fminf(1.0f, y / len)));
			int row = (int)((0.5f - el / 3.14159265f) * RTX_SKY_H);
			int col = (int)((atan2f(z, x) / 6.2831853f + 0.5f) * RTX_SKY_W);
			row = row < 0 ? 0 : (row >= (int)RTX_SKY_H ? (int)RTX_SKY_H - 1 : row);
			col = col < 0 ? 0 : (col >= (int)RTX_SKY_W ? (int)RTX_SKY_W - 1 : col);
			sum[row][col][0] += v->Color.r / 255.0f;
			sum[row][col][1] += v->Color.g / 255.0f;
			sum[row][col][2] += v->Color.b / 255.0f;
			sum[row][col][3] += 1.0f;
			n++;
		}
	}
	if (n == 0)
	{
		// clear colours: 0 top half, 1 bottom half
		for (int half = 0; half < 2; half++)
		{
			if (!level->clearColor[half].enable)
			{
				continue;
			}
			for (int row = half * RTX_SKY_H / 2; row < (half + 1) * (int)RTX_SKY_H / 2; row++)
			{
				for (int col = 0; col < (int)RTX_SKY_W; col++)
				{
					for (int k = 0; k < 3; k++)
					{
						sum[row][col][k] = (u8)level->clearColor[half].rgb[k] / 255.0f;
					}
					sum[row][col][3] = 1.0f;
					n++;
				}
			}
		}
	}
	RTX_HDR->skyValid = (n > 0);
	if (n == 0)
	{
		return;
	}
	// empty cells: nearest filled cell (rows first, then around the row)
	for (int row = 0; row < (int)RTX_SKY_H; row++)
	{
		for (int col = 0; col < (int)RTX_SKY_W; col++)
		{
			const float *src = NULL;
			for (int d = 0; (d < (int)(RTX_SKY_H + RTX_SKY_W)) && (src == NULL); d++)
			{
				for (int dr = -d; (dr <= d) && (src == NULL); dr++)
				{
					int r = row + dr, c = (col + (d - abs(dr))) % (int)RTX_SKY_W;
					if ((r >= 0) && (r < (int)RTX_SKY_H) && (sum[r][c][3] > 0.0f))
					{
						src = sum[r][c];
					}
				}
			}
			for (int k = 0; k < 3; k++)
			{
				RTX_HDR->sky[row][col][k] = src ? src[k] / src[3] : 0.0f;
			}
		}
	}
}

// ---- the traced floor under the karts -----------------------------------------
// The PS1 draws a kart over the floor it stands on (no depth buffer), so where
// the physics puts it a little off the drawn floor nobody sees it. Traced, the
// wheels sink into the floor in one place and float over it in the next: each
// kart is set onto the traced level mesh by its wheels (Rtx_SeatKart). A 2D
// grid of the level's floor-like triangles answers "how high is the floor here"

#define RTX_FLOOR_CELL_MAX  (512 * 512)
#define RTX_FLOOR_LIST_MAX  (1 << 21)
static struct
{
	float minX, minZ, cell;
	int dimX, dimZ;
	u32 start[RTX_FLOOR_CELL_MAX + 1];
	u32 list[RTX_FLOOR_LIST_MAX];
	const struct RtxLevelVert *verts;
	const struct RtxLevelTri *tris;
} s_floor;

static void Rtx_BuildFloorGrid(const struct RtxLevelVert *verts, int numVerts, const struct RtxLevelTri *tris, int numTris)
{
	s_floor.verts = verts;
	s_floor.tris = tris;
	s_floor.dimX = s_floor.dimZ = 0;
	if (numVerts <= 0)
	{
		return;
	}
	float minX = 1e30f, minZ = 1e30f, maxX = -1e30f, maxZ = -1e30f;
	for (int i = 0; i < numVerts; i++)
	{
		minX = fminf(minX, verts[i].pos[0]);
		maxX = fmaxf(maxX, verts[i].pos[0]);
		minZ = fminf(minZ, verts[i].pos[2]);
		maxZ = fmaxf(maxZ, verts[i].pos[2]);
	}
	float cell = 256.0f;
	while (((maxX - minX) / cell + 1.0f) * ((maxZ - minZ) / cell + 1.0f) > (float)RTX_FLOOR_CELL_MAX)
	{
		cell *= 2.0f;
	}
	s_floor.minX = minX;
	s_floor.minZ = minZ;
	s_floor.cell = cell;
	s_floor.dimX = (int)((maxX - minX) / cell) + 1;
	s_floor.dimZ = (int)((maxZ - minZ) / cell) + 1;
	int cells = s_floor.dimX * s_floor.dimZ;
	memset(s_floor.start, 0, sizeof(u32) * (size_t)(cells + 1));
	// two passes: count per cell, then fill (floor-like triangles only)
	for (int pass = 0; pass < 2; pass++)
	{
		for (int t = 0; t < numTris; t++)
		{
			const float *a = verts[tris[t].v[0]].pos, *b = verts[tris[t].v[1]].pos, *c = verts[tris[t].v[2]].pos;
			float ux = b[0] - a[0], uy = b[1] - a[1], uz = b[2] - a[2], vx = c[0] - a[0], vy = c[1] - a[1], vz = c[2] - a[2];
			float nx = uy * vz - uz * vy, ny = uz * vx - ux * vz, nz = ux * vy - uy * vx;
			if (fabsf(ny) < 0.35f * sqrtf(nx * nx + ny * ny + nz * nz))
			{
				continue; // walls
			}
			int x0 = (int)((fminf(a[0], fminf(b[0], c[0])) - minX) / cell), x1 = (int)((fmaxf(a[0], fmaxf(b[0], c[0])) - minX) / cell);
			int z0 = (int)((fminf(a[2], fminf(b[2], c[2])) - minZ) / cell), z1 = (int)((fmaxf(a[2], fmaxf(b[2], c[2])) - minZ) / cell);
			for (int z = z0; z <= z1; z++)
			{
				for (int x = x0; x <= x1; x++)
				{
					u32 k = (u32)(z * s_floor.dimX + x);
					if (pass == 0)
					{
						s_floor.start[k + 1]++;
					}
					else if (s_floor.start[k] < RTX_FLOOR_LIST_MAX)
					{
						s_floor.list[s_floor.start[k]++] = (u32)t;
					}
				}
			}
		}
		if (pass == 0)
		{
			for (int k = 0; k < cells; k++)
			{
				s_floor.start[k + 1] += s_floor.start[k];
			}
		}
		else
		{
			// (filling advanced each start to the next cell's: shift back)
			for (int k = cells; k > 0; k--)
			{
				s_floor.start[k] = s_floor.start[k - 1];
			}
			s_floor.start[0] = 0;
		}
	}
}

// height of the traced floor at (x, z) nearest to y within range; 0 = none
static int Rtx_FloorHeight(float x, float y, float z, float range, float *h)
{
	if (s_floor.dimX <= 0)
	{
		return 0;
	}
	int cx = (int)((x - s_floor.minX) / s_floor.cell), cz = (int)((z - s_floor.minZ) / s_floor.cell);
	if ((cx < 0) || (cz < 0) || (cx >= s_floor.dimX) || (cz >= s_floor.dimZ))
	{
		return 0;
	}
	u32 k = (u32)(cz * s_floor.dimX + cx);
	int found = 0;
	float best = range;
	for (u32 i = s_floor.start[k]; (i < s_floor.start[k + 1]) && (i < RTX_FLOOR_LIST_MAX); i++)
	{
		const struct RtxLevelTri *t = &s_floor.tris[s_floor.list[i]];
		const float *a = s_floor.verts[t->v[0]].pos, *b = s_floor.verts[t->v[1]].pos, *c = s_floor.verts[t->v[2]].pos;
		// barycentrics in the xz plane
		float d = (b[2] - c[2]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[2] - c[2]);
		if (fabsf(d) < 1e-6f)
		{
			continue;
		}
		float wa = ((b[2] - c[2]) * (x - c[0]) + (c[0] - b[0]) * (z - c[2])) / d;
		float wb = ((c[2] - a[2]) * (x - c[0]) + (a[0] - c[0]) * (z - c[2])) / d;
		float wc = 1.0f - wa - wb;
		if ((wa < -1e-3f) || (wb < -1e-3f) || (wc < -1e-3f))
		{
			continue;
		}
		float hy = wa * a[1] + wb * b[1] + wc * c[1];
		if (fabsf(hy - y) < best)
		{
			best = fabsf(hy - y);
			*h = hy;
			found = 1;
		}
	}
	return found;
}

// a kart's wheels were just captured (dynamic triangles [wheelStart, wheelEnd),
// two per wheel quad): the lowest point of each wheel against the traced floor
// under it; grounded, the kart (wheels and body) moves by their mean gap
static void Rtx_SeatKart(const void *inst, int wheelStart, int wheelEnd)
{
	struct RtxDynTri *dyn = Rtx_DynTris();
	float gapSum = 0.0f;
	int n = 0, wheels = 0;
	for (int t = wheelStart; t + 1 < wheelEnd; t += 2)
	{
		const float *low = dyn[t].p[0];
		for (int q = t; q < t + 2; q++)
		{
			for (int c = 0; c < 3; c++)
			{
				low = (dyn[q].p[c][1] < low[1]) ? dyn[q].p[c] : low;
			}
		}
		float h;
		wheels++;
		if (Rtx_FloorHeight(low[0], low[1], low[2], 60.0f, &h))
		{
			gapSum += low[1] - h;
			n++;
		}
	}
	float target = 0.0f;
	if (n >= 2)
	{
		float gap = gapSum / (float)n;
		s_rtx.statGapN++;
		s_rtx.statGapSum += gap;
		s_rtx.statGapAbs += fabsf(gap);
		target = (fabsf(gap) < 40.0f) ? -gap : 0.0f; // (off the ground: as it is)
	}
	s_rtx.statGapFar += (n < 2) && (wheels > 0);

	// smoothed per kart (a step in the floor doesn't make it jump)
	int slot = -1, oldest = 0;
	for (int i = 0; i < 16; i++)
	{
		if (s_rtx.kartLift[i].inst == inst)
		{
			slot = i;
			break;
		}
		oldest = (s_rtx.kartLift[i].gen < s_rtx.kartLift[oldest].gen) ? i : oldest;
	}
	if ((slot < 0) || (s_rtx.kartLift[slot].gen + 1u != s_rtx.gen))
	{
		slot = (slot < 0) ? oldest : slot;
		s_rtx.kartLift[slot].inst = inst;
		s_rtx.kartLift[slot].dy = target;
	}
	else
	{
		s_rtx.kartLift[slot].dy += (target - s_rtx.kartLift[slot].dy) * 0.5f;
	}
	s_rtx.kartLift[slot].gen = s_rtx.gen;
	float dy = s_rtx.kartLift[slot].dy;
	if (dy == 0.0f)
	{
		return;
	}
	for (int t = wheelStart; t < wheelEnd; t++)
	{
		dyn[t].p[0][1] += dy;
		dyn[t].p[1][1] += dy;
		dyn[t].p[2][1] += dy;
	}
	for (int b = 0; b < s_rtx.numKartBody; b++)
	{
		if (s_rtx.kartBody[b].inst != inst)
		{
			continue;
		}
		s_rtx.statGapBody++;
		for (int t = s_rtx.kartBody[b].start; t < s_rtx.kartBody[b].end; t++)
		{
			dyn[t].p[0][1] += dy;
			dyn[t].p[1][1] += dy;
			dyn[t].p[2][1] += dy;
		}
	}
}

// every frame of every animated texture (boosters, lava, screens): their
// polygons among the level as the PS1 drew it are told by tpage, clut and uvs
static void Rtx_FindAnimatedTextures(void)
{
	s_rtx.numAnimRect = 0;
	for (int i = 0; i < s_rtx.numTexSlots; i++)
	{
		u32 ptr = s_rtx.texSrc[i];
		if ((ptr & 3) != 1)
		{
			continue;
		}
		const struct AnimTex *anim = (const struct AnimTex *)(ptr - 1);
		struct IconGroup4 **frames = ANIMTEX_GETARRAY(anim);
		for (int f = 0; (f < anim->numFrames) && (f < 64); f++)
		{
			const struct IconGroup4 *g = frames[f];
			if (g == NULL)
			{
				continue;
			}
			const struct TextureLayout *lods[4] = {&g->far, &g->middle, &g->near, &g->mosaic};
			for (int l = 0; (l < 4) && (s_rtx.numAnimRect < 1024); l++)
			{
				const struct TextureLayout *t = lods[l];
				u8 us[4] = {t->u0, t->u1, t->u2, t->u3}, vs[4] = {t->v0, t->v1, t->v2, t->v3};
				u8 u0 = 255, v0 = 255, u1 = 0, v1 = 0;
				for (int k = 0; k < 4; k++)
				{
					u0 = (us[k] < u0) ? us[k] : u0;
					u1 = (us[k] > u1) ? us[k] : u1;
					v0 = (vs[k] < v0) ? vs[k] : v0;
					v1 = (vs[k] > v1) ? vs[k] : v1;
				}
				if ((u1 <= u0) || (v1 <= v0))
				{
					continue;
				}
				s_rtx.animRect[s_rtx.numAnimRect].tpage = t->tpage;
				s_rtx.animRect[s_rtx.numAnimRect].clut = t->clut;
				s_rtx.animRect[s_rtx.numAnimRect].u0 = u0;
				s_rtx.animRect[s_rtx.numAnimRect].v0 = v0;
				s_rtx.animRect[s_rtx.numAnimRect].u1 = u1;
				s_rtx.animRect[s_rtx.numAnimRect].v1 = v1;
				s_rtx.numAnimRect++;
			}
		}
	}
}

static int Rtx_IsAnimatedTexture(u16 tpage, u16 clut, const u8 (*uv)[2])
{
	int u0 = 255, v0 = 255, u1 = 0, v1 = 0;
	for (int k = 0; k < 3; k++)
	{
		u0 = (uv[k][0] < u0) ? uv[k][0] : u0;
		u1 = (uv[k][0] > u1) ? uv[k][0] : u1;
		v0 = (uv[k][1] < v0) ? uv[k][1] : v0;
		v1 = (uv[k][1] > v1) ? uv[k][1] : v1;
	}
	for (int i = 0; i < s_rtx.numAnimRect; i++)
	{
		// (blend bits aside: the PS1 may draw a page semi-transparent)
		if ((s_rtx.animRect[i].clut == clut) && ((s_rtx.animRect[i].tpage & 0xff9f) == (tpage & 0xff9f)) && (u0 + 1 >= s_rtx.animRect[i].u0) &&
		    (u1 <= s_rtx.animRect[i].u1 + 1) && (v0 + 1 >= s_rtx.animRect[i].v0) && (v1 <= s_rtx.animRect[i].v1 + 1))
		{
			return 1;
		}
	}
	return 0;
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
			// as DrawLevelOvr1P_WriteProjectedUv: bit 23 mirrors the texture (uv 0<->1, 2<->3),
			// bit 7 makes the face a triangle (no 4th vertex)
			int mirror = (tableWord & 0x00800000u) != 0;
			int triangle = (tableWord & 0x80u) != 0;
			int slotVert[4];
			for (int s = 0; s < 4; s++)
			{
				u32 shift = (tableWord >> ((3 - s) * 8)) & 0x1f;
				int k = (24 - (int)shift) / 8;
				slotVert[s] = block->index[sRtxGridFace[f][k & 3]];
			}

			int texSlot = isWater[q] && (waterSlot >= 0) ? waterSlot : Rtx_TexSlotFor((u32)block->ptr_texture_mid[f]);
			int animated = !isWater[q] && (((u32)block->ptr_texture_mid[f] & 1) != 0);
			int terrain = block->terrain_type;
			int glossy = (terrain == TERRAIN_ICY_ROAD) || (terrain == TERRAIN_METAL);
			int ice = (terrain == TERRAIN_ICE);
			static const int triSlots[2][3] = {{0, 1, 2}, {1, 3, 2}};
			static const u8 mirrorCorner[4] = {1, 0, 3, 2};
			for (int t = 0; t < (triangle ? 1 : 2); t++)
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
					tri->corner[c] = mirror ? mirrorCorner[triSlots[t][c]] : (u8)triSlots[t][c];
				}
				if (!ok || (tri->v[0] == tri->v[1]) || (tri->v[1] == tri->v[2]) || (tri->v[0] == tri->v[2]))
				{
					continue;
				}
				tri->texSlot = (u16)(texSlot < 0 ? 0xffff : texSlot);
				tri->flags = (texSlot >= 0 ? RTX_TRI_TEXTURED : 0) | ((drawOrderLow & 0x80000000u) ? RTX_TRI_DOUBLE_SIDED : 0) |
				             (isWater[q] ? RTX_TRI_WATER : 0) | (animated ? RTX_TRI_ANIMATED : 0) | (glossy ? RTX_TRI_GLOSSY : 0) |
				             (ice ? RTX_TRI_ICE : 0);
				numTris++;
			}
		}
	}

	Rtx_BuildFloorGrid(verts, numVerts, tris, numTris);
	Rtx_FindAnimatedTextures();

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
	Rtx_UploadSky(level);
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
	s_rtx.otBase = (u32)pb->ptrOT;
	s_rtx.uiOtStart = (u32)gGT->pushBuffer_UI.ptrOT;
	s_rtx.uiOtEnd = (u32)gGT->pushBuffer_UI.renderBucketOTRangeEnd;
	s_rtx.cameraValid = 1;
}

// ---- GPU side ----------------------------------------------------------------

int Rtx_FrameActive(void)
{
	return (RTX_HDR != NULL) && s_rtx.enabled && RTX_HDR->rendererReady && s_rtx.cameraValid && s_rtx.levelValid;
}

// The level's LOD distances (texture detail, the far low-detail list) are tuned
// for the PS1's 512 pixel wide screen: at the output resolution a texel covers
// fewer pixels, so the same detail per pixel reaches out that much further.
// The texture layer takes its texels from what the PS1 drew, so this is what
// sharpens the distance (CTR_RTX_LOD_SCALE overrides, 1 = as the PS1)
// the level's texture detail threshold this frame (view z; MainFrame_RenderFrame):
// past it the PS1 draws its middle / far textures, the renderer takes the
// level mesh's near ones there instead of the texture layer (no texture pop-in)
void Rtx_SetTextureNearDepth(int depth)
{
	if (RTX_HDR != NULL)
	{
		RTX_HDR->texNearDepth = (float)depth;
	}
}

int Rtx_LevelLodScale(int depth)
{
	if ((RTX_HDR == NULL) || !s_rtx.enabled || !RTX_HDR->rendererReady || !s_rtx.levelValid)
	{
		return depth;
	}
	static float scale;
	if (scale == 0.0f)
	{
		const char *env = SDL_getenv("CTR_RTX_LOD_SCALE");
		scale = env ? (float)atof(env) : -1.0f;
	}
	float k = scale;
	if (k <= 0.0f)
	{
		int w, h;
		NativeRenderer_RtxViewportSize(&w, &h);
		k = (float)w / 512.0f;
	}
	k = fminf(fmaxf(k, 1.0f), 4.0f);
	float d = (float)depth * k;
	return (d > 2.0e9f) ? 2000000000 : (int)d;
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
// Returns which commands were captured (bits = word offsets, RTX_MASK_ALL when
// GL can skip the whole packet); semi-transparent ones stay with GL.
static u32 Rtx_CapturePacketTo(u32 *words, int numWords, u16 serial, int allowSemi, struct RtxDynTri *out, int *num, int max)
{
	int i = 0;
	int polys = 0;
	int left = 0; // commands GL still has to draw
	u32 mask = 0;

	while (i < numWords)
	{
		u32 cmd = words[i];
		u32 code = cmd >> 24;

		if ((code == 0) || ((code >= 0xe1) && (code <= 0xe6)))
		{
			i++;
			continue;
		}
		if ((code < 0x20) || (code >= 0x40))
		{
			left++;
			break; // lines, sprites: GL draws the rest of the packet
		}

		int gouraud = (code & 0x10) != 0;
		int quad = (code & 0x08) != 0;
		int textured = (code & 0x04) != 0;
		int raw = (code & 0x01) != 0;
		int n = quad ? 4 : 3;
		int size = 1 + n * (1 + textured) + (gouraud ? n - 1 : 0);
		if (i + size > numWords)
		{
			left++;
			break;
		}
		if (((code & 2) && !(allowSemi && (code & 4))) || (i >= 32))
		{
			s_rtx.statSemi += (serial != RTX_LEVEL_SERIAL);
			left++; // semi-transparent: GL (depth tested against the traced image)
			i += size;
			continue;
		}
		int startDyn = *num;

		float pos[4][3];
		u8 uv[4][2];
		u8 col[4][4];
		u16 clut = 0, tpage = 0;
		u32 xys[4];
		const float *views[4];
		for (int v = 0, w2 = i + 1; v < n; v++)
		{
			w2 += (v == 0 || !gouraud) ? 0 : 1;
			xys[v] = words[w2++];
			w2 += textured;
		}
		if (!Rtx_CapPolygon(xys, n, serial, views))
		{
			s_rtx.statNoProj += (serial != RTX_LEVEL_SERIAL);
			goto skip;
		}
		int w = i + 1;
		for (int v = 0; v < n; v++)
		{
			u32 c = (v == 0 || !gouraud) ? cmd : words[w++];
			col[v][0] = raw ? 0x80 : (u8)c;
			col[v][1] = raw ? 0x80 : (u8)(c >> 8);
			col[v][2] = raw ? 0x80 : (u8)(c >> 16);
			col[v][3] = 0xff;

			w++; // xy: views[v]
			Rtx_ViewToWorld(views[v], pos[v]);

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
			if (*num >= max)
			{
				goto skip;
			}
			struct RtxDynTri *d = &out[(*num)++];
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
			d->flags = (textured ? RTX_TRI_TEXTURED : 0) | ((code & 2) ? RTX_TRI_SEMI : 0);
			d->flags |= (textured && (serial == RTX_LEVEL_SERIAL) && Rtx_IsAnimatedTexture(tpage, clut, d->uv)) ? RTX_TRI_ANIMATED : 0;
			s_rtx.statCapAnimated += (d->flags & RTX_TRI_ANIMATED) != 0;
			d->object = serial;
		}
		polys++;
		s_rtx.statCaptured += (serial != RTX_LEVEL_SERIAL);
		mask |= 1u << i;
		i += size;
		continue;

	skip:
		*num = startDyn; // not projected (or full): GL draws this one
		left++;
		i += size;
	}
	if (polys == 0)
	{
		return 0;
	}
	return (left == 0) ? RTX_MASK_ALL : mask;
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

static void Rtx_AddEmitter(const float *pos, const float *color, float weight, float radius, int glowModel)
{
	float lum = 0.299f * color[0] + 0.587f * color[1] + 0.114f * color[2];
	float mx = fmaxf(color[0], fmaxf(color[1], color[2])), mn = fminf(color[0], fminf(color[1], color[2]));
	s_rtx.statEmitDim += (lum < 0.05f);
	s_rtx.statEmitGrey += (lum >= 0.05f) && ((mx - mn < 0.35f * mx) && (weight >= 0.5f) && !glowModel);
	// white / grey glows (exhaust puffs, sparkles, highlights) don't light the scene:
	// lit from the kart itself they draw a bright outline around it (white smoke
	// particles add themselves, weakly: Rtx_OnParticle). Glowing models (turbo
	// flames burn white-hot) do: trace.comp keeps their light off models
	s_rtx.statEmitFull += (s_rtx.numEmitters >= RTX_MAX_EMITTERS);
	if ((lum < 0.05f) || ((mx - mn < 0.35f * mx) && (weight >= 0.5f) && !glowModel) || (s_rtx.numEmitters >= RTX_MAX_EMITTERS))
	{
		return;
	}
	s_rtx.statEmitAdded++;
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
		float tex[3] = {1, 1, 1};
		s_rtx.statEmitNoProj += !found;
		s_rtx.statEmitGlow += found && anyBlend;
		s_rtx.statEmitSemi += found && !anyBlend && semi;
		s_rtx.statEmitOpaque += found && !anyBlend && !semi;
		int haveTex = found && (!textured || Rtx_AverageTexels(tpage, clut, uv, n, tex));
		s_rtx.statPolyNoTex += found && !haveTex;
		if (haveTex)
		{
			float c[3] = {col[0] * tex[0], col[1] * tex[1], col[2] * tex[2]};
			float mx = fmaxf(c[0], fmaxf(c[1], c[2])), mn = fminf(c[0], fminf(c[1], c[2]));
			int vivid = (mx >= 0.5f) && ((mx - mn) >= 0.35f * mx); // bright and coloured
			// additive polygons, and other see-through ones that are vivid (glows,
			// flames); opaque paint never (karts, characters)
			if (anyBlend || (semi && ((blend == 1) || vivid)))
			{
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
				Rtx_AddEmitter(centre, c, 0.5f + radius * radius / 1500.0f, radius, anyBlend); // bigger flame, more light
			}
			else
			{
				s_rtx.statPolyNotGlow++;
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
	s_rtx.statEmitParticle++;
	// fire, sparks, glows: additive sprites, or bright coloured ones (the colour
	// word carries no reliable semi-transparency bit here); grey smoke is skipped
	int additive = ((t->tpage >> 5) & 3) == 1;
	Rtx_CaptureCamera(pb);
	u8 uv[4][2] = {{t->u0, t->v0}, {t->u1, t->v1}, {t->u2, t->v2}, {t->u3, t->v3}};
	float tex[3];
	if (!Rtx_AverageTexels(t->tpage, t->clut, uv, 4, tex))
	{
		s_rtx.statPartNoTex++;
		return;
	}
	s_rtx.statPartAdditive += additive;
	float view[3] = {viewX4 * 0.25f, viewY4 * 0.25f, viewZ4 * 0.25f}, pos[3];
	Rtx_ViewToWorld(view, pos);
	float c[3] = {(u8)color / 128.0f * tex[0], (u8)(color >> 8) / 128.0f * tex[1], (u8)(color >> 16) / 128.0f * tex[2]};
	if (!additive)
	{
		float mx = fmaxf(c[0], fmaxf(c[1], c[2])), mn = fminf(c[0], fminf(c[1], c[2]));
		if ((mx >= 0.6f) && ((mx - mn) < 0.35f * mx))
		{
			s_rtx.statPartWhite++;
			Rtx_AddEmitter(pos, c, 0.25f, 40.0f, 0); // white smoke: a faint glow
			return;
		}
		if ((mx < 0.3f) || ((mx - mn) < 0.35f * mx))
		{
			s_rtx.statPartDull++;
			return; // dim or grey (smoke, dust)
		}
		s_rtx.statPartVivid++;
	}
	Rtx_AddEmitter(pos, c, 1.0f, 40.0f, 0);
}

// This frame's lights against the remembered ones: a light seen in the same
// place for a while is static (a torch, a brazier, a statue's fire) and lights
// from memory from then on, also while the game doesn't draw it (off screen,
// behind the camera) for up to 30 s, until it is on screen nearby and not seen
// (gone out). Lights that move (karts' flames) are this frame's only. Writes
// the strongest (by brightness at the camera) of both into out
static int Rtx_RememberLights(const struct RtxPointLight *cur, int numCur, struct RtxPointLight *out, int maxOut)
{
	const int numMem = (int)(sizeof(s_rtx.remembered) / sizeof(s_rtx.remembered[0]));
	// (a glow's centre jumps around from frame to frame: flicker, sparks; what
	// tells a torch from a kart's flame is how far it gets from where it was
	// first seen within its first second)
	const float matchDist = 250.0f, staticMatchDist = 200.0f, moveDist = 300.0f;
	const int staticFrames = 30;
	const u32 forgetFrames = 900, levelGen = RTX_HDR->levelGen;
	struct RtxRememberedLight *mem = s_rtx.remembered;
	for (int m = 0; m < numMem; m++)
	{
		mem[m].matched = 0;
		if (mem[m].levelGen != levelGen)
		{
			mem[m].seen = 0; // (another level's)
			mem[m].lastGen = 0;
		}
	}

	static u8 fromMem[RTX_MAX_DYN_LIGHTS * 4];
	for (int c = 0; c < numCur; c++)
	{
		const struct RtxPointLight *l = &cur[c];
		int best = -1;
		float bestD2 = matchDist * matchDist;
		for (int m = 0; m < numMem; m++)
		{
			if ((mem[m].seen == 0) || mem[m].matched)
			{
				continue;
			}
			float dx = mem[m].pos[0] - l->pos[0], dy = mem[m].pos[1] - l->pos[1], dz = mem[m].pos[2] - l->pos[2];
			float d2 = dx * dx + dy * dy + dz * dz;
			// (a static light only takes what is where it is: a kart's flame going
			// past a torch is not the torch moving)
			int isStatic = !mem[m].moving && (mem[m].seen >= staticFrames);
			if ((d2 < bestD2) && (!isStatic || (d2 < staticMatchDist * staticMatchDist)))
			{
				bestD2 = d2;
				best = m;
			}
		}
		if (best < 0)
		{
			// a new one: a free slot, else the longest unseen
			best = 0;
			for (int m = 0; m < numMem; m++)
			{
				if (mem[m].seen == 0)
				{
					best = m;
					break;
				}
				best = ((s_rtx.gen - mem[m].lastGen) > (s_rtx.gen - mem[best].lastGen)) ? m : best;
			}
			memset(&mem[best], 0, sizeof(mem[best]));
			memcpy(mem[best].pos, l->pos, sizeof(l->pos));
			memcpy(mem[best].anchor, l->pos, sizeof(l->pos));
			memcpy(mem[best].color, l->color, sizeof(l->color));
			mem[best].falloff = l->falloff;
			mem[best].size = l->size;
			mem[best].levelGen = levelGen;
		}
		struct RtxRememberedLight *r = &mem[best];
		float ax = l->pos[0] - r->anchor[0], ay = l->pos[1] - r->anchor[1], az = l->pos[2] - r->anchor[2];
		int moved = (r->seen < staticFrames) && (ax * ax + ay * ay + az * az > moveDist * moveDist);
		s_rtx.statMemMoved += moved && !r->moving;
		r->moving |= moved;
		float k = r->moving ? 1.0f : 0.15f; // static: smoothed (the flames flicker, their light less)
		for (int j = 0; j < 3; j++)
		{
			r->pos[j] += (l->pos[j] - r->pos[j]) * (r->moving ? 1.0f : ((r->seen < staticFrames) ? 0.5f : 0.05f));
			r->color[j] += (l->color[j] - r->color[j]) * k;
		}
		r->falloff += (l->falloff - r->falloff) * k;
		r->size += (l->size - r->size) * k;
		r->seen = (r->seen < 60000) ? r->seen + 1 : r->seen;
		r->lastGen = s_rtx.gen;
		r->matched = 1;
		fromMem[c] = !r->moving && (r->seen >= staticFrames);
	}

	// candidates: this frame's moving / new ones, every static remembered one
	float cam[3];
	const float zero[3] = {0, 0, 0};
	Rtx_ViewToWorld(zero, cam);
	static struct RtxPointLight cand[RTX_MAX_DYN_LIGHTS * 4 + 128];
	static float score[RTX_MAX_DYN_LIGHTS * 4 + 128];
	int numCand = 0;
	for (int c = 0; c < numCur; c++)
	{
		if (!fromMem[c])
		{
			cand[numCand++] = cur[c];
		}
	}
	const float *R = RTX_HDR->camR, *T = RTX_HDR->camT;
	for (int m = 0; m < numMem; m++)
	{
		struct RtxRememberedLight *r = &mem[m];
		if ((r->seen < staticFrames) || r->moving)
		{
			// not (yet) static: forget it soon after it's gone
			r->seen = ((r->seen != 0) && (s_rtx.gen - r->lastGen > 30u)) ? 0 : r->seen;
			continue;
		}
		if (!r->matched)
		{
			// unseen: gone out if it should be on screen nearby, forgotten after a while
			float v[3];
			for (int i = 0; i < 3; i++)
			{
				v[i] = R[i * 3] * r->pos[0] + R[i * 3 + 1] * r->pos[1] + R[i * 3 + 2] * r->pos[2] + T[i];
			}
			float sx = (v[2] > 1.0f) ? v[0] / v[2] * RTX_HDR->projH : 1e9f, sy = (v[2] > 1.0f) ? v[1] / v[2] * RTX_HDR->projH : 1e9f;
			int onScreen = (v[2] > 100.0f) && (v[2] < 2500.0f) && (fabsf(sx) < 0.4f * RTX_HDR->vpW) && (fabsf(sy) < 0.4f * RTX_HDR->vpH);
			if ((onScreen && (s_rtx.gen - r->lastGen > 4u)) || (s_rtx.gen - r->lastGen > forgetFrames))
			{
				s_rtx.statMemGone += onScreen;
				s_rtx.statMemOld += !onScreen;
				r->seen = 0;
				continue;
			}
		}
		struct RtxPointLight *l = &cand[numCand++];
		memset(l, 0, sizeof(*l));
		memcpy(l->pos, r->pos, sizeof(l->pos));
		memcpy(l->color, r->color, sizeof(l->color));
		l->falloff = r->falloff;
		l->size = r->size;
	}

	// the strongest at the camera first
	for (int c = 0; c < numCand; c++)
	{
		const struct RtxPointLight *l = &cand[c];
		float dx = l->pos[0] - cam[0], dy = l->pos[1] - cam[1], dz = l->pos[2] - cam[2];
		float reach = 3.0f * l->falloff;
		score[c] = (l->color[0] + l->color[1] + l->color[2]) / (1.0f + (dx * dx + dy * dy + dz * dz) / (reach * reach));
	}
	int n = 0;
	while ((n < maxOut) && (n < numCand))
	{
		int best = n;
		for (int c = n + 1; c < numCand; c++)
		{
			best = (score[c] > score[best]) ? c : best;
		}
		struct RtxPointLight tl = cand[n];
		float ts = score[n];
		cand[n] = cand[best];
		score[n] = score[best];
		cand[best] = tl;
		score[best] = ts;
		out[n] = cand[n];
		n++;
	}
	return n;
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

	// this frame's lights: colour = average colour of what glows; more glowing
	// stuff = brighter
	static struct RtxPointLight cur[RTX_MAX_DYN_LIGHTS * 4];
	int numCur = numClusters;
	for (int c = 0; c < numCur; c++)
	{
		const struct RtxEmitter *k = &clusters[c];
		struct RtxPointLight *l = &cur[c];
		memset(l, 0, sizeof(*l));
		float strength = 1.2f * sqrtf(k->weight);
		strength = strength > 6.0f ? 6.0f : strength;
		for (int j = 0; j < 3; j++)
		{
			l->pos[j] = k->pos[j] / k->weight;
			l->color[j] = k->color[j] / k->weight * strength;
		}
		l->falloff = 300.0f + 4.0f * k->radius;
		l->size = 10.0f + 0.25f * k->radius;
	}
	int n = Rtx_RememberLights(cur, numCur, RTX_HDR->dynLights, (int)RTX_MAX_DYN_LIGHTS);
	RTX_HDR->numDynLights = (u32)n;
	s_rtx.statLights += n;
}

// ---- GL overlay depth -------------------------------------------------------
// What GL still draws over the ray traced image (glowing and transparent models,
// particles, skidmarks, sprites) gets a view z per vertex; the overlay shaders
// drop fragments behind the traced surface (NativeRenderer_RtxSetImage).

// every packet of the OT walk, OT entries included: which ordering table and
// bucket the following packets hang off
void Rtx_TrackPacket(u32 *packet)
{
	u32 a = (u32)packet;
	if ((s_rtx.otBase != 0) && (a >= s_rtx.otBase) && (a < s_rtx.otBase + 0x400 * 4))
	{
		s_rtx.ctxWorld = 1;
		s_rtx.ctxBucket = (int)((a - s_rtx.otBase) >> 2);
	}
	else if ((s_rtx.uiOtStart != 0) && (a >= s_rtx.uiOtStart) && (a <= s_rtx.uiOtEnd))
	{
		s_rtx.ctxWorld = 0; // HUD: always on top
	}
}

static float Rtx_BucketZ(void)
{
	return s_rtx.ctxWorld ? s_rtx.zPerBucket * (s_rtx.ctxBucket + 0.5f) : 0.0f;
}

float Rtx_VertexDepth(u32 xy)
{
	if (!s_rtx.ctxActive || !Rtx_FrameActive())
	{
		return 0.0f;
	}
	if (s_rtx.ctxZ > 0.0f)
	{
		return s_rtx.ctxZ;
	}
	const float *v = Rtx_CapLookup(xy, s_rtx.ctxSerial);
	if ((v != NULL) && (v[2] > 1.0f))
	{
		return v[2];
	}
	return Rtx_BucketZ();
}

// a smoke particle (POLY_FT4 sprite, not additive): its screen corners back at
// its depth into world space, a camera-facing quad that shadows partly
static void Rtx_CaptureSmoke(u32 *packet, int lengthWords, float z)
{
	u32 code = (lengthWords >= 9) ? (packet[1] >> 24) : 0;
	if (((code & 0xfc) != 0x2c) || (s_rtx.partH <= 0.0f) || (z <= 1.0f) || (s_rtx.numSmoke + 2 > (int)(sizeof(s_rtx.smoke) / sizeof(s_rtx.smoke[0]))))
	{
		return;
	}
	u16 clut = (u16)(packet[3] >> 16), tpage = (u16)(packet[5] >> 16);
	if (((tpage >> 5) & 3) == 1)
	{
		return; // additive: fire, sparks, glows (they light, they don't shade)
	}
	float pos[4][3];
	u8 uv[4][2];
	for (int v = 0; v < 4; v++)
	{
		u32 xy = packet[2 + v * 2], t = packet[3 + v * 2];
		float view[3] = {((s16)(xy & 0xffff) - s_rtx.partOfx) * z / s_rtx.partH, ((s16)(xy >> 16) - s_rtx.partOfy) * z / s_rtx.partH, z};
		Rtx_ViewToWorld(view, pos[v]);
		uv[v][0] = (u8)t;
		uv[v][1] = (u8)(t >> 8);
	}
	static const int triIdx[2][3] = {{0, 1, 2}, {1, 3, 2}};
	for (int t = 0; t < 2; t++)
	{
		struct RtxDynTri *d = &s_rtx.smoke[s_rtx.numSmoke++];
		for (int c = 0; c < 3; c++)
		{
			int v = triIdx[t][c];
			memcpy(d->p[c], pos[v], sizeof(d->p[c]));
			d->uv[c][0] = uv[v][0];
			d->uv[c][1] = uv[v][1];
			d->col[c][0] = (u8)packet[1];
			d->col[c][1] = (u8)(packet[1] >> 8);
			d->col[c][2] = (u8)(packet[1] >> 16);
			d->col[c][3] = 0xff;
		}
		d->clut = clut;
		d->tpage = tpage;
		d->flags = RTX_TRI_TEXTURED | RTX_TRI_SMOKE;
		d->object = 0xfffe;
	}
}

int Rtx_ClassifyPacket(u32 *packet, int lengthWords)
{
	const struct RtxRange *r = Rtx_FindRange((u32)packet);
	s_rtx.ctxSerial = 0;
	s_rtx.ctxZ = 0.0f;
	s_rtx.ctxActive = 0;
	s_rtx.ctxMask = 0;
	if (r == NULL)
	{
		s_rtx.ctxActive = s_rtx.ctxWorld; // world effects: projections recorded outside tags, else the OT bucket
		return RTX_PACKET_DRAW;
	}
	switch (r->tag)
	{
	case RTX_TAG_LEVEL:
		// the exact PS1 texturing of what is on screen: the renderer's primary
		// rays take texels from these (next frame), lighting from the level mesh
		if (s_rtx.cameraValid && (lengthWords > 0))
		{
			Rtx_CapturePacketTo(packet + 1, lengthWords, RTX_LEVEL_SERIAL, 1, (struct RtxDynTri *)(s_rtx.base + RTX_OFF_LEVEL_CAP), &s_rtx.numLevelCap,
			                    (int)RTX_MAX_LEVEL_CAP);
		}
		return RTX_PACKET_SKIP;
	case RTX_TAG_SHADOW:
		return RTX_PACKET_SKIP;
	case RTX_TAG_SKY:
		return RTX_PACKET_SKY;
	case RTX_TAG_INSTANCE:
	{
		s_rtx.ctxActive = 1;
		s_rtx.ctxSerial = r->serial;
		// first vertex of a polygon packet: calibrates bucket -> z for packets without projections
		u32 code = (lengthWords >= 2) ? (packet[1] >> 24) : 0;
		if (s_rtx.ctxWorld && (code >= 0x20) && (code < 0x40))
		{
			const float *v = Rtx_CapLookup(packet[2], r->serial);
			if ((v != NULL) && (v[2] > 1.0f))
			{
				s_rtx.calZ += v[2];
				s_rtx.calB += s_rtx.ctxBucket + 0.5;
				s_rtx.calN++;
			}
		}
		u32 mask = Rtx_CapturedMask((u32)packet);
		if (mask == RTX_MASK_ALL)
		{
			return RTX_PACKET_SKIP;
		}
		s_rtx.ctxMask = mask;
		return RTX_PACKET_DRAW;
	}
	case RTX_TAG_DEPTH:
		s_rtx.ctxActive = 1;
		s_rtx.ctxZ = r->z;
		Rtx_CaptureSmoke(packet, lengthWords, r->z);
		return RTX_PACKET_DRAW;
	default:
		return RTX_PACKET_DRAW; // HUD models, other views
	}
}

// one particle's packet (words long, at prim): its view z for the depth test
void Rtx_OnParticleDepth(struct PushBuffer *pb, u32 *prim, int words, s32 viewZ4, s32 ofx, s32 ofy, s32 h)
{
	if ((RTX_HDR == NULL) || !s_rtx.enabled || (pb != &sdata->gGT->pushBuffer[0]))
	{
		return;
	}
	s_rtx.partOfx = ofx / 65536.0f;
	s_rtx.partOfy = ofy / 65536.0f;
	s_rtx.partH = (float)h;
	u32 start = (u32)prim;
	struct RtxRange *r = (s_rtx.numRanges > 0) ? &s_rtx.ranges[s_rtx.numRanges - 1] : NULL;
	if ((r == NULL) || (r->tag != RTX_TAG_DEPTH) || (r->start != start))
	{
		if (s_rtx.numRanges >= RTX_MAX_RANGES)
		{
			return;
		}
		// (the last particle was culled after its hook: its slot is reused)
		r = &s_rtx.ranges[s_rtx.numRanges++];
		if ((s_rtx.numRanges > 1) && (start < r[-1].start))
		{
			s_rtx.rangesSorted = 0;
		}
	}
	r->start = start;
	r->end = start + (u32)words * 4;
	r->tag = RTX_TAG_DEPTH;
	r->serial = 0;
	r->allowSemi = 0;
	r->mode = 0;
	r->z = viewZ4 * 0.25f;
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
	const float *depth = NULL;
	NativeRenderer_RtxViewportSize(&w, &h);
	int ok = Rtx_RenderFrame(w, h, &image, &depth);
	NativeRenderer_RtxSetImage(ok ? image : NULL, ok ? depth : NULL, w, h);
}

int Rtx_RenderFrame(int outW, int outH, const u8 **rgba, const float **depth)
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
		printf("[RTX]   models: %d instances, polygons %d traced, %d unprojected (GL), %d see-through (GL); %d level polygons\n", s_rtx.statInstances,
		       s_rtx.statCaptured, s_rtx.statNoProj, s_rtx.statSemi, RTX_HDR->numLevelCap);
		printf("[RTX]   (last 300 frames) instances: %d traced, %d glowing, %d GL only; emitter candidates: %d glow-model polys, %d see-through, %d opaque (skipped), %d particles, %d no projection; rejected %d grey, %d dim\n",
		       s_rtx.statInstMode[RTX_INST_CAPTURE], s_rtx.statInstMode[RTX_INST_GLOW], s_rtx.statInstMode[RTX_INST_GL], s_rtx.statEmitGlow, s_rtx.statEmitSemi,
		       s_rtx.statEmitOpaque, s_rtx.statEmitParticle, s_rtx.statEmitNoProj, s_rtx.statEmitGrey, s_rtx.statEmitDim);
		printf("[RTX]   particles: %d additive, %d vivid, %d white, %d dull (no light), %d no texels\n", s_rtx.statPartAdditive, s_rtx.statPartVivid,
		       s_rtx.statPartWhite, s_rtx.statPartDull, s_rtx.statPartNoTex);
		printf("[RTX]   polygons: %d no texels, %d not glowing; emitters added %d (%d over the limit) -> %d frame lights\n", s_rtx.statPolyNoTex, s_rtx.statPolyNotGlow,
		       s_rtx.statEmitAdded, s_rtx.statEmitFull, s_rtx.statLights);
		int remembered = 0, tracked = 0, moving = 0;
		for (int m = 0; m < (int)(sizeof(s_rtx.remembered) / sizeof(s_rtx.remembered[0])); m++)
		{
			const struct RtxRememberedLight *r = &s_rtx.remembered[m];
			remembered += (r->seen >= 30) && !r->moving;
			tracked += (r->seen != 0);
			moving += (r->seen != 0) && r->moving;
		}
		printf("[RTX]   karts on the traced floor: %d seated (%d bodies moved along), mean gap %.1f (abs %.1f), %d with no floor under their wheels; %d animated texture rects, %d animated level polygons drawn\n",
		       s_rtx.statGapN, s_rtx.statGapBody, s_rtx.statGapN ? s_rtx.statGapSum / s_rtx.statGapN : 0.0, s_rtx.statGapN ? s_rtx.statGapAbs / s_rtx.statGapN : 0.0, s_rtx.statGapFar,
		       s_rtx.numAnimRect, s_rtx.statCapAnimated);
		s_rtx.statGapN = s_rtx.statGapFar = s_rtx.statGapBody = s_rtx.statCapAnimated = 0;
		s_rtx.statGapSum = s_rtx.statGapAbs = 0.0;
		printf("[RTX]   glows tracked %d: %d static (remembered), %d moving; %d moved / %d forgotten on screen / %d timed out (last 300 frames)\n", tracked, remembered, moving,
		       s_rtx.statMemMoved, s_rtx.statMemGone, s_rtx.statMemOld);
		s_rtx.statMemMoved = s_rtx.statMemGone = s_rtx.statMemOld = 0;
		s_rtx.statPolyNoTex = s_rtx.statPolyNotGlow = s_rtx.statEmitAdded = s_rtx.statEmitFull = s_rtx.statLights = 0;
		memset(s_rtx.statInstMode, 0, sizeof(s_rtx.statInstMode));
		s_rtx.statEmitGlow = s_rtx.statEmitSemi = s_rtx.statEmitOpaque = s_rtx.statEmitParticle = s_rtx.statEmitGrey = s_rtx.statEmitDim = s_rtx.statEmitNoProj = 0;
		s_rtx.statPartAdditive = s_rtx.statPartVivid = s_rtx.statPartWhite = s_rtx.statPartDull = s_rtx.statPartNoTex = 0;
		for (int i = 0; i < (int)RTX_HDR->numDynLights && i < 8; i++)
		{
			const struct RtxPointLight *l = &RTX_HDR->dynLights[i];
			printf("[RTX]   frame light %d: %.0f %.0f %.0f colour %.2f %.2f %.2f falloff %.0f\n", i, l->pos[0], l->pos[1], l->pos[2], l->color[0], l->color[1], l->color[2], l->falloff);
		}
		const struct PrimMem *pm = &sdata->gGT->backBuffer->primMem;
		printf("[RTX]   primitive memory %u of %u KB, level LOD x%.2f\n", (u32)((u32)pm->cursor - (u32)pm->start) / 1024, (u32)pm->capacityBytes / 1024,
		       Rtx_LevelLodScale(1000) / 1000.0f);
		for (int i = 0; i < s_rtx.numDyn && i < 3; i++)
		{
			struct RtxDynTri *d = &Rtx_DynTris()[i];
			printf("[RTX]   tri %d obj %d: %.0f %.0f %.0f / %.0f %.0f %.0f / %.0f %.0f %.0f\n", i, d->object, d->p[0][0], d->p[0][1], d->p[0][2], d->p[1][0],
			       d->p[1][1], d->p[1][2], d->p[2][0], d->p[2][1], d->p[2][2]);
		}
	}
	// last frame's smoke after this frame's models
	int smoke = s_rtx.numSmoke < (int)RTX_MAX_DYN_TRIS - s_rtx.numDyn ? s_rtx.numSmoke : (int)RTX_MAX_DYN_TRIS - s_rtx.numDyn;
	memcpy(&Rtx_DynTris()[s_rtx.numDyn], s_rtx.smoke, sizeof(struct RtxDynTri) * smoke);
	s_rtx.numDyn += smoke;
	s_rtx.numSmoke = 0;
	RTX_HDR->numDynTris = (u32)s_rtx.numDyn;
	RTX_HDR->numLevelCap = (u32)s_rtx.numLevelCap; // last frame's; this frame's walk refills the section
	s_rtx.numLevelCap = 0;
	RTX_HDR->outW = (u32)outW;
	RTX_HDR->outH = (u32)outH;
	RTX_HDR->dispW = 512.0f;
	RTX_HDR->dispH = 216.0f;
	RTX_HDR->debugView = (u32)s_rtx.debugView;
	RTX_HDR->lightMode = s_rtx.lightMode ? 0u : 1u; // default full relight, F5 hybrid
	RTX_HDR->settings = s_rtx.settings;
	// pipelined: show the previous request's image (made the same way, at this
	// size, the game frame before) and only wait for the renderer to take this
	// frame's input
	int pipelined = s_rtx.settings.pipelined && s_rtx.prevPipelined && (s_rtx.prevW == outW) && (s_rtx.prevH == outH) && (s_rtx.prevGen + 1u == s_rtx.gen);
	RTX_HDR->frame++;

	__sync_synchronize();
	u32 req = RTX_HDR->frameReq + 1;
	RTX_HDR->frameReq = req;
	Rtx_FutexWake(&RTX_HDR->frameReq);

	u64 start = SDL_GetTicksNS(), lastBusy = start;
	volatile u32 *wait = pipelined ? &RTX_HDR->frameTaken : &RTX_HDR->frameDone;
	s_rtx.prevPipelined = 0;
	for (;;)
	{
		u32 v = *wait;
		if (v == req)
		{
			break;
		}
		// (a level being loaded / prepared takes a while: up to 15 s; the frame
		// after it still comes, so 250 ms from when it was last seen busy)
		u64 now = SDL_GetTicksNS(), waited = now - start;
		lastBusy = RTX_HDR->rendererBusy ? now : lastBusy;
		if (((now - lastBusy) > 250000000ull) || (waited > 15000000000ull))
		{
			static int warned;
			if (!warned++)
			{
				printf("[RTX] renderer timed out (request %u, game frame %u, %u ms), frame drawn without ray tracing\n", req, RTX_HDR->frame, (u32)(waited / 1000000ull));
			}
			return 0;
		}
		if (kill(s_rtx.rendererPid, 0) != 0)
		{
			printf("[RTX] renderer exited, ray tracing off\n");
			RTX_HDR->rendererReady = 0;
			return 0;
		}
		Rtx_FutexWait(wait, v, 2000000); // (the renderer wakes it)
	}
	__sync_synchronize();
	s_rtx.prevPipelined = s_rtx.settings.pipelined;
	s_rtx.prevW = outW;
	s_rtx.prevH = outH;
	s_rtx.prevGen = s_rtx.gen;
	u32 slot = (pipelined ? req - 1u : req) & 1u;
	*rgba = s_rtx.base + RTX_OFF_IMAGE + slot * RTX_SLOT_BYTES;
	*depth = (const float *)(s_rtx.base + RTX_OFF_DEPTH + slot * RTX_SLOT_BYTES);
	return 1;
}
