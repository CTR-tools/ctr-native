/*
 * Derived from REDRIVER2/PsyCross MIT source:
 * externals/PsyCross/include/PsyX/PsyX_render.h
 * See THIRD_PARTY_NOTICES.md for copyright and license details.
 */

#ifndef NATIVE_RENDERER_TYPES_H
#define NATIVE_RENDERER_TYPES_H

#include <macros.h>
#include <psx/libgte.h>
#include <psx/libgpu.h>

#define LUT_WIDTH              (256)
#define LUT_HEIGHT             (256)

#define VRAM_WIDTH             (1024)
#define VRAM_HEIGHT            (512)


// Max detail subdivides all level geometry, and one 1P frame can pass 64K
// vertices; split screen multiplies that. Exceeding this truncates the frame's
// vertex list (see NativeRenderer_UpdateVertexBuffer), which drops geometry
// rather than misplacing it.
#define MAX_VERTEX_BUFFER_SIZE (1u << 19)

typedef struct
{
	// Sub-pixel when PGXP recovered the GTE vertex, otherwise retail integers.
	float x, y;
	// Perspective divisor for PGXP polygons; 0 keeps retail affine mapping.
	float w;
	// Texture page and CLUT are per-polygon constants. Moving them out of the
	// position attribute keeps them out of the interpolated varyings, so scaling
	// clip w cannot perturb the texture lookup.
	s16 page, clut;

	u8 u, v, bright, dither;
	u8 r, g, b, a;

	s8 tcx, tcy, _p0, _p1;
	u16 _pad;
} GrVertex;

CTR_STATIC_ASSERT(sizeof(GrVertex) == 32);

typedef enum
{
	a_position,
	a_texcoord,
	a_color,
	a_extra,
	a_page_clut,
} ShaderAttrib;

typedef enum
{
	BM_NONE,
	BM_AVERAGE,
	BM_ADD,
	BM_SUBTRACT,
	BM_ADD_QUATER_SOURCE
} BlendMode;

typedef enum
{
	TF_4_BIT,
	TF_8_BIT,
	TF_16_BIT,

	TF_32_BIT_RGBA
} TexFormat;

typedef u32 TextureID;
typedef u32 ShaderID;

#endif
