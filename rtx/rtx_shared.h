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
 * image[]. The static level mesh is written once per level (levelGen).
 *
 * Fixed-size plain data only: identical layout in 32-bit and 64-bit builds.
 */

#include <stdint.h>

#define RTX_SHM_NAME "/ctr_rtx"
#define RTX_MAGIC    0x58545243u // 'CRTX'
#define RTX_VERSION  2u

#define RTX_MAX_LEVEL_VERTS 65536u
#define RTX_MAX_LEVEL_TRIS  (1u << 19)
#define RTX_MAX_TEX_SLOTS   16384u
#define RTX_MAX_DYN_TRIS    65536u
#define RTX_MAX_DYN_LIGHTS  64u
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
	uint32_t pad[12];

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
#define RTX_OFF_VRAM          RTX_ALIGN(RTX_OFF_DYN_TRIS + RTX_MAX_DYN_TRIS * sizeof(struct RtxDynTri))
#define RTX_OFF_IMAGE         RTX_ALIGN(RTX_OFF_VRAM + RTX_VRAM_W * RTX_VRAM_H * 2u)
#define RTX_SHM_SIZE          RTX_ALIGN(RTX_OFF_IMAGE + RTX_MAX_W * RTX_MAX_H * 4u)

#endif
