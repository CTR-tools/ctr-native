#ifndef RTX_SHARED_H
#define RTX_SHARED_H

/*
 * Shared memory between the 32-bit game (native/build/ctr_native) and the
 * 64-bit ray tracer (native/build/ctr_rtx). NVIDIA's 32-bit Vulkan driver has
 * no ray-tracing extensions, so tracing lives in its own 64-bit process.
 *
 * Per frame the game writes the camera, the animated texture slots, the
 * dynamic triangles (karts, items, scenery models: captured from the GTE) and
 * PSX VRAM, bumps frameReq and waits for frameDone == frameReq. The renderer
 * traces and writes an RGBA8 image (alpha = primary ray hit geometry) at
 * image slot frameReq & 1 and the view z of each pixel's opaque hit in the
 * same depth slot (the GL overlay depth-tests against it). The static level
 * mesh is written once per level (levelGen).
 *
 * Pipelined (RtxSettings.pipelined) the game only waits for frameTaken ==
 * frameReq (the renderer has copied this frame's input) and shows the previous
 * request's image: tracing overlaps the game's next frame, one frame late.
 * Both sides sleep on the counters with futexes (shared mapping: across
 * processes, 32-bit and 64-bit alike).
 *
 * Fixed-size plain data only: identical layout in 32-bit and 64-bit builds.
 */

#include <stdint.h>

#define RTX_SHM_NAME "/ctr_rtx"
#define RTX_MAGIC    0x58545243u // 'CRTX'
#define RTX_VERSION  8u

#define RTX_MAX_LEVEL_VERTS 65536u
#define RTX_MAX_LEVEL_TRIS  (1u << 19)
#define RTX_MAX_TEX_SLOTS   16384u
#define RTX_MAX_DYN_TRIS    65536u
#define RTX_MAX_DYN_LIGHTS  64u
#define RTX_MAX_LEVEL_CAP   131072u // level polygons the PS1 drew last frame (exact textures)
#define RTX_SKY_W           8u      // sky radiance table: azimuth
#define RTX_SKY_H           8u      // elevation (top row first)
#define RTX_MAX_W           3840u
#define RTX_MAX_H           2160u
#define RTX_VRAM_W          1024u
#define RTX_VRAM_H          512u

enum RtxTriFlags
{
	RTX_TRI_TEXTURED = 1,     // sample VRAM (else untextured: vertex colour is the colour)
	RTX_TRI_DOUBLE_SIDED = 2, // level quads with the double-sided bit
	RTX_TRI_WATER = 4,        // reflective
	RTX_TRI_EMISSIVE = 8,     // unlit (vertex colour x texel shown as is)
	RTX_TRI_ANIMATED = 16,    // animated texture (AnimTex: boosters, lava, flames, screens)
	RTX_TRI_GLOSSY = 32,      // terrain icy road, metal: glossy reflections
	RTX_TRI_ICE = 64,         // terrain ice (and, renderer side, see-through bluish glass on icy levels): 90% mirror
	RTX_TRI_SEMI = 128,       // captured polygon drawn semi-transparent (blend from its tpage)
	RTX_TRI_SMOKE = 256,      // a smoke particle (camera-facing quad): only shadows, partly
	// water samples its texture slot (the level's water env map) by world position
	// level triangles are semi-transparent when their texture's blend bits are
	// not 3 (overlay 226 draws those with code 0x3e), water always
};

// level vertex: world position (level units, Y up) + PS1 vertex colour
struct RtxLevelVert
{
	float pos[3];
	uint32_t rgba; // color_hi, 0x80 = neutral
};

// level triangle: vertex indices + texture slot + which TextureLayout corner
// (0 TL, 1 TR, 2 BL, 3 BR) each vertex samples
struct RtxLevelTri
{
	uint16_t v[3];
	uint16_t texSlot;
	uint8_t corner[3];
	uint8_t flags; // RtxTriFlags
};

// resolved TextureLayout (corner UVs + clut + tpage), refreshed every frame so
// animated textures (AnimTex) follow the game
struct RtxTexSlot
{
	uint8_t uv[4][2];
	uint16_t clut;
	uint16_t tpage;
};

// dynamic triangle, world space
struct RtxDynTri
{
	float p[3][3];
	uint8_t uv[3][2];
	uint16_t clut;
	uint16_t tpage;
	uint8_t col[3][4];
	uint16_t flags;  // RtxTriFlags
	uint16_t object; // serial of the instance it came from, per frame
};

struct RtxPointLight
{
	float pos[3]; // base on the floor; renderer lifts it by height (clamped under the ceiling)
	float height;
	float color[3];
	float falloff;
	float size;
	float pad[3];
};

#define RTX_MAX_LIGHTS 1024u

// renderer settings: the game's F12 overlay (keys 1-8) and CTR_RTX_* env vars;
// the renderer reads them every frame
struct RtxSettings
{
	uint32_t renderScale; // percent of the output size that is traced (TAA upscales)
	uint32_t quality;     // samples per pixel preset: 0 low 2, 1 medium 4, 2 high 8, 3 ultra 16
	float budgetMs;       // trace pass ceiling: only ever lowers the preset's samples
	uint32_t taa;         // temporal anti-aliasing / upscaling of the final image
	uint32_t texFilter;   // bilinear PS1 texels on camera hits (colour key aware)
	uint32_t specular;    // GGX highlights and reflections on every material
	uint32_t fog;         // height / distance haze and sun shafts
	uint32_t pipelined;   // renderer a frame behind the game (one frame of latency)
};

// renderer pass timings (ms, averaged), RtxHeader.passMs
enum RtxPass
{
	RTX_PASS_BUILD,  // uploads, model / texture layer BLAS, TLAS
	RTX_PASS_TRACE,
	RTX_PASS_CACHE,
	RTX_PASS_ATROUS,
	RTX_PASS_FINAL,  // composite, TAA, bloom
	RTX_PASS_COPY,   // image + depth readback
	RTX_PASS_CPU,    // renderer CPU work outside the GPU wait
	RTX_PASS_GPU,    // whole command buffer
	RTX_PASS_COUNT
};

struct RtxLighting
{
	uint32_t valid;   // 0 = renderer defaults
	uint32_t indoor;  // no sun; ambient + point lights
	uint32_t numLights;
	uint32_t pad0;
	float sunDir[3];  // towards the sun, world space
	float sunCone;    // radians
	float sunColor[3];
	float pad1;
	float skyZenith[3];
	float pad2;
	float skyHorizon[3];
	float pad3;
	float ground[3];
	float pad4;
	float ambient[3]; // indoor
	float exposure;
	struct RtxPointLight lights[RTX_MAX_LIGHTS];
};

struct RtxHeader
{
	uint32_t magic;
	uint32_t version;
	volatile uint32_t frameReq;  // game: frame published
	volatile uint32_t frameDone; // renderer: image for frameReq written
	volatile uint32_t gamePid;
	volatile uint32_t rendererPid;
	volatile uint32_t rendererReady; // Vulkan up, ray tracing available
	volatile uint32_t quit;
	volatile uint32_t frameTaken; // renderer: input of frameReq copied (the game may write the next frame's)
	volatile uint32_t rendererBusy; // renderer: loading / preparing a level (the game waits longer)

	// level (rewritten when levelGen changes)
	uint32_t levelGen;
	uint32_t numLevelVerts;
	uint32_t numLevelTris;
	uint32_t numTexSlots;
	int32_t levelID;
	int32_t redirectBase; // custom track bigfile base, -1 = stock level
	char lightsKey[64];   // assets/rtx_lights/<key>.json

	// frame
	uint32_t frame;
	uint32_t numDynTris;
	uint32_t outW, outH;   // image size wanted (window viewport)
	float camR[9];         // GTE camera: view = camR * world + camT (row-major)
	float camT[3];
	float projH;           // GTE H (distanceToScreen)
	float vpX, vpY, vpW, vpH; // 3D viewport in PS1 screen coords (pushBuffer rect)
	float dispW, dispH;    // PS1 display size the output image covers
	uint32_t debugView;    // 0 final, 1 albedo, 2 normal, 3 direct, 4 indirect
	uint32_t resetHistory;
	float renderMs;        // renderer: last frame GPU+copy time
	uint32_t numDynLights; // emitters seen this frame: flames, turbos, glowing particles
	uint32_t lightMode;    // 0 hybrid: the track's own PS1 lighting + traced extras, 1 full relight (F5)
	uint32_t traceSpp;     // renderer: samples per pixel it is currently using
	uint32_t numLevelCap;  // level polygons drawn by the PS1 path last frame (RtxDynTri, world space)
	uint32_t skyValid;     // sky[] holds the level's sky colours
	struct RtxSettings settings;
	float passMs[RTX_PASS_COUNT]; // renderer: averaged pass timings (RtxPass)
	uint32_t inW, inH;            // renderer: traced (internal) size
	float texNearDepth;           // view z past which the PS1 draws the level with its lower textures (0 = none)
	float sky[RTX_SKY_H][RTX_SKY_W][3]; // display colours of the level's skybox / gradient by direction

	struct RtxPointLight dynLights[RTX_MAX_DYN_LIGHTS];
};

_Static_assert(sizeof(struct RtxLevelVert) == 16, "RtxLevelVert layout");
_Static_assert(sizeof(struct RtxLevelTri) == 12, "RtxLevelTri layout");
_Static_assert(sizeof(struct RtxTexSlot) == 12, "RtxTexSlot layout");
_Static_assert(sizeof(struct RtxDynTri) == 64, "RtxDynTri layout");
_Static_assert(sizeof(struct RtxPointLight) == 48, "RtxPointLight layout");

// byte offsets of each section in the shared mapping
#define RTX_ALIGN(x)          (((x) + 4095u) & ~4095u)
#define RTX_OFF_HEADER        0u
#define RTX_OFF_LEVEL_VERTS   RTX_ALIGN(sizeof(struct RtxHeader))
#define RTX_OFF_LEVEL_TRIS    RTX_ALIGN(RTX_OFF_LEVEL_VERTS + RTX_MAX_LEVEL_VERTS * sizeof(struct RtxLevelVert))
#define RTX_OFF_TEX_SLOTS     RTX_ALIGN(RTX_OFF_LEVEL_TRIS + RTX_MAX_LEVEL_TRIS * sizeof(struct RtxLevelTri))
#define RTX_OFF_DYN_TRIS      RTX_ALIGN(RTX_OFF_TEX_SLOTS + RTX_MAX_TEX_SLOTS * sizeof(struct RtxTexSlot))
#define RTX_OFF_LEVEL_CAP     RTX_ALIGN(RTX_OFF_DYN_TRIS + RTX_MAX_DYN_TRIS * sizeof(struct RtxDynTri))
#define RTX_OFF_VRAM          RTX_ALIGN(RTX_OFF_LEVEL_CAP + RTX_MAX_LEVEL_CAP * sizeof(struct RtxDynTri))
#define RTX_SLOT_BYTES        (RTX_MAX_W * RTX_MAX_H * 4u) // one image / depth slot (two of each: frameReq & 1)
#define RTX_OFF_IMAGE         RTX_ALIGN(RTX_OFF_VRAM + RTX_VRAM_W * RTX_VRAM_H * 2u)
#define RTX_OFF_DEPTH         RTX_ALIGN(RTX_OFF_IMAGE + 2u * RTX_SLOT_BYTES)
#define RTX_SHM_SIZE          RTX_ALIGN(RTX_OFF_DEPTH + 2u * RTX_SLOT_BYTES)

#endif
