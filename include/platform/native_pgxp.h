#ifndef NATIVE_PGXP_H
#define NATIVE_PGXP_H

#include <macros.h>
#include <platform/native_config.h>
#include <stddef.h>

// NOTE: Precision geometry transform pipeline, modelled on DuckStation's PGXP
// (Stenzek, https://github.com/stenzek/duckstation), itself descended from
// iCatButler's original PGXP. This is a separate implementation written for the
// decompiled source, following CameronRedmore's Crash Team Racing: Turbocharged.
// RTPS/RTPT truncate every projected vertex to an integer SXY before the CPU
// writes it into a GPU packet. That truncation is what makes PS1 polygons
// wobble as the camera moves, and the missing depth is why the GPU can only map
// textures affinely. Native keeps the sub-pixel screen position and view depth
// of each projection next to the retail integer, follows it through the GTE SXY
// FIFO and the memory words game code stores it into, and lets the GPU parser
// look it up by packet address. Every lookup is validated against the integer
// the packet actually holds, so anything the game computed some other way falls
// back to retail data.

static inline int NativePgxp_Mode(void)
{
	const int mode = g_config.pgxpMode;

	// NativeConfig_Load falls back to atoi() for an unrecognised enum name, so
	// config.ini can hold a number outside the enum.
	return ((mode >= PGXP_MODE_OFF) && (mode <= PGXP_MODE_PERSPECTIVE)) ? mode : PGXP_MODE_OFF;
}

#define NATIVE_PGXP_ACTIVE()          (NativePgxp_Mode() != PGXP_MODE_OFF)
#define NATIVE_PGXP_GEOMETRY_ACTIVE() (NativePgxp_Mode() >= PGXP_MODE_GEOMETRY)
#define NATIVE_PGXP_TEXTURES_ACTIVE() (NativePgxp_Mode() >= PGXP_MODE_PERSPECTIVE)

typedef struct
{
	// Sub-pixel screen position, in the drawing space the GTE projected into.
	float x;
	float y;
	// Perspective divisor, in the same units as the GTE's IR3 (MAC3 >> 12). Only
	// its ratio across a polygon matters, since the GPU divides it back out.
	// <= 0 when the projection had no usable depth, which keeps the polygon on
	// retail affine texture mapping.
	float w;
	// Retail packed SXY (x low, y high) this vertex was truncated to. Every
	// lookup compares it, so a stale shadow can never apply precision to a
	// packet word that holds a different vertex.
	u32 value;
} NativePgxpVertex;

// gGT->db[0] and db[1] allocate their primitive memory separately, so the
// shadow tables cover two host ranges rather than one.
enum
{
	NATIVE_PGXP_PRIM_REGION_COUNT = 2
};

void NativePgxp_SetPrimRegion(int index, const void *start, size_t size);

// CPU-side precision (like DuckStation's "PGXP CPU"): the vertices CTR builds
// itself rather than reading from the level. Retail builds subdivision
// midpoints, LOD fades and near-plane clip vertices from truncated integers, so
// they sit slightly off the edges they split; at sub-pixel projection that
// opens a visible crack against neighbours drawn without the split.
//
// An exact position is keyed by the address of an s16 xyz triplet and only
// trusted while that triplet still holds the integers it was stored with.
void NativePgxp_SetPosition(const void *key, const s16 *vector, const float *precise);
void NativePgxp_ClearPosition(const void *key);
// Returns 1 with the exact position, or 0 with `vector` converted to float.
int NativePgxp_GetPosition(const void *key, const s16 *vector, float *out);
void NativePgxp_CopyPosition(const void *dstKey, const void *srcKey, const s16 *vector);

// GTE input vectors V0..V2. Loading a vector register drops its exact value, so
// set this after the MTC2s that load the vertex. RTPS/RTPT use it while the
// integers still match.
void NativePgxp_GteSetInput(int slot, const s16 *vector, const float *precise);
int NativePgxp_GteGetInput(int slot, s16 vx, s16 vy, s16 vz, double *out);

// GTE side: mirror of the SXY0..SXY2 FIFO.
void NativePgxp_GteProject(float x, float y, float w, u32 value);
void NativePgxp_GteWriteSXY(int reg, u32 value);
void NativePgxp_GteReadSXY(int reg, u32 value);
// NCLIP from the precise SXY0..SXY2; returns 0 when any of them is unknown.
int NativePgxp_GteNclip(u32 sxy0, u32 sxy1, u32 sxy2, s64 *out);

// Memory side: `dst` is the address of a packed SXY word, or of a primitive's x
// field, and `value` the packed SXY being stored there.
void NativePgxp_StoreGteSXY(const void *dst, int reg, u32 value);
void NativePgxp_CopyXY(const void *dst, const void *src, u32 value);
// Bind by value alone, for the game code that writes an MFC2 result straight
// into a packet without routing it through a register store.
void NativePgxp_BindWrittenXY(const void *dst, u32 value);

// HUD and menu drawing never goes through the GTE, so it must not bind its own
// screen coordinates to a world vertex that happens to share the same integer.
void NativePgxp_SetWorldPhase(int active);

// Loading a vector register drops its exact value, so set the precise input after
// the MTC2s that load the vertex.
void NativePgxp_GteInvalidateInput(int reg);

// GPU side.
int NativePgxp_Lookup(const void *addr, u32 value, NativePgxpVertex *out);
void NativePgxp_EndFrame(void);

#endif