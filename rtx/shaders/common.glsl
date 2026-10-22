// Shared declarations for the ctr_rtx compute shaders. Buffer layouts mirror
// rtx/rtx_shared.h (read as raw uint words; no 8/16-bit storage needed).

#extension GL_EXT_ray_query : require

layout(set = 0, binding = 0) uniform accelerationStructureEXT tlas;

layout(set = 0, binding = 1, std430) readonly buffer FrameBuf
{
	vec4 rInv[3];     // world = rInv * (view - camT)
	vec4 camPos;      // xyz world camera position
	vec4 camR[3];     // view = camR * world + camT
	vec4 camT;
	vec4 prevR[3];
	vec4 prevT;
	vec4 proj;        // H, vpX, vpY, vpW
	vec4 proj2;       // vpH, dispW, dispH, prevH
	uvec4 size;       // outW, outH, frameIndex, debugView
	vec4 sunDir;      // xyz towards sun, w cone (radians)
	vec4 sunColor;    // rgb, w exposure
	vec4 skyZenith;   // rgb, w vertex colour mix
	vec4 skyHorizon;  // rgb, w ray epsilon
	vec4 groundColor; // rgb, w history max
	vec4 ambient;     // rgb (indoor), w indoor flag
	uvec4 counts;     // numStaticLights, numLevelTris, numDynTris, resetHistory
	vec4 params;      // aoRadius, time (seconds), 1: level corner normals oriented (back-face test), unused
	uvec4 extra;      // numDynLights (after the static lights), unused, lightMode (0 hybrid, 1 full), samples per pixel
	vec4 params2;     // fog density at the camera's height, cache base cell size, fog height falloff, near texture depth (view z, 0 none)
	uvec4 extra2;     // level corner normals base (in LevelNormals), cache entries (power of 2), cache write phase (0..3), unused
	vec4 gridMin;     // static light grid: xyz origin, w cell size
	uvec4 gridDim;    // cells per axis, w: sky table valid
	vec4 sky[64];     // sky radiance by direction: 8 elevations (top first) x 8 azimuths
	uvec4 outSize;    // output (TAA) w, h, TAA on, unused; size above is the traced (internal) size
	vec4 jitter;      // this frame's camera ray offset within its pixel (0..1, TAA on), unused
	uvec4 feat;       // texture filter, specular, fog, unused (RtxSettings)
} F;

layout(set = 0, binding = 2, std430) readonly buffer LevelVerts { uint levelVerts[]; };   // 4 words per vertex
layout(set = 0, binding = 3, std430) readonly buffer LevelTris { uint levelTris[]; };     // 3 words per triangle
layout(set = 0, binding = 4, std430) readonly buffer TexSlots { uint texSlots[]; };       // 3 words per slot
layout(set = 0, binding = 5, std430) readonly buffer DynTris { uint dynTris[]; };         // 16 words per triangle
layout(set = 0, binding = 6, std430) readonly buffer Vram { uint vram[]; };               // 1024x512 u16
layout(set = 0, binding = 7, std430) buffer Lights { vec4 lights[]; };                    // 3 vec4 per light
layout(set = 0, binding = 17, std430) readonly buffer LevelNormals { vec4 levelNormals[]; }; // smooth: per vertex, then per triangle corner from F.extra2.x
layout(set = 0, binding = 18, std430) readonly buffer DynNormals { vec4 dynNormals[]; };     // smooth, 3 per triangle
layout(set = 0, binding = 16, std430) buffer LumSums { uint lumSums[]; };                  // exposure: PS1 reference, traced, samples
layout(set = 0, binding = 21, std430) buffer CacheKeys { uint cacheKeys[]; };              // radiance cache: key, last frame written
layout(set = 0, binding = 22, std430) buffer CacheAccum { uint cacheAccum[]; };            // this frame's sums: r, g, b (x256), count
layout(set = 0, binding = 23, std430) buffer CacheVal { vec4 cacheVal[]; };                // indirect light, w = frames of data
layout(set = 0, binding = 24, std430) readonly buffer LightGrid { uint lightGrid[]; };    // per cell (offset, count), then static light indices
layout(set = 0, binding = 26, std430) readonly buffer CapTris { uint capTris[]; };        // level polygons as the PS1 drew them, 16 words each
layout(set = 0, binding = 31, std430) readonly buffer LevelTint { vec4 levelTint[]; };   // per vertex: the art's colour beyond the traced light (linear), w glow
layout(set = 0, binding = 39, std430) readonly buffer BlueNoise { float blueNoise[]; };  // 64x64 void-and-cluster ranks, 0..1

const uint INST_LEVEL = 0u;
const uint INST_DYN = 1u;
const uint INST_CAP = 2u; // texture layer only (primary rays): mask 0x4
const uint MASK_SCENE = 0x3u;
const uint MASK_FAST = 0xau; // models + the level forced opaque (ctr_rtx.c): bounce rays

// set while tracing a diffuse bounce: its rays (and its hit's shadow rays) see
// the level as opaque, settled by the RT hardware. Indirect light is soft:
// colour-keyed holes and see-through polygons count as solid there
bool g_fastRays = false;

const uint TRI_TEXTURED = 1u;
const uint TRI_DOUBLE_SIDED = 2u;
const uint TRI_WATER = 4u;
const uint TRI_EMISSIVE = 8u;
const uint TRI_ANIMATED = 16u;
const uint TRI_GLOSSY = 32u;
const uint TRI_ICE = 64u;
const uint TRI_SEMI = 128u;
const uint TRI_SMOKE = 256u;
const vec3 ICE_TINT = vec3(0.78, 0.9, 1.0);

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

uint vram16(int x, int y)
{
	uint idx = uint(clamp(y, 0, 511)) * 1024u + uint(x & 1023);
	uint w = vram[idx >> 1];
	return ((idx & 1u) != 0u) ? (w >> 16) : (w & 0xffffu);
}

// PS1 texel at integer uv: rgb in 0..1, a = 0 transparent (0x0000)
vec4 psxTexel(uint tpage, uint clut, int u, int v)
{
	int mode = int((tpage >> 7) & 3u);
	int bx = int(tpage & 15u) * 64;
	int by = int((tpage >> 4) & 1u) * 256;
	int cx = int(clut & 63u) * 16;
	int cy = int((clut >> 6) & 511u);
	u &= 255;
	v &= 255;
	uint c;
	if (mode == 0)
	{
		uint w = vram16(bx + (u >> 2), by + v);
		uint idx = (w >> (uint(u & 3) * 4u)) & 15u;
		c = vram16(cx + int(idx), cy);
	}
	else if (mode == 1)
	{
		uint w = vram16(bx + (u >> 1), by + v);
		uint idx = (w >> (uint(u & 1) * 8u)) & 255u;
		c = vram16(cx + int(idx), cy);
	}
	else
	{
		c = vram16(bx + u, by + v);
	}
	vec3 rgb = vec3(float(c & 31u), float((c >> 5) & 31u), float((c >> 10) & 31u)) / 31.0;
	return vec4(rgb, (c == 0u) ? 0.0 : 1.0);
}

// set around camera hits (trace.comp): their texels are filtered (F.feat.x)
bool g_texFilter = false;

// bilinear PS1 texels at uv t, the taps clamped to the polygon's texels [lo, hi]
// (no bleeding from its texture page neighbours); colour-keyed taps don't count
// (no dark fringe at cut-outs), whether it is see-through stays the nearest
// texel's as the PS1 draws it
vec4 psxTexelFiltered(uint tpage, uint clut, vec2 t, vec2 lo, vec2 hi)
{
	vec2 b = t - 0.5;
	vec2 f = floor(b);
	vec2 w = b - f;
	vec3 sum = vec3(0.0);
	float wsum = 0.0;
	for (int i = 0; i < 4; i++)
	{
		vec2 o = vec2(float(i & 1), float(i >> 1));
		vec2 q = clamp(f + o, lo, hi);
		vec4 c = psxTexel(tpage, clut, int(q.x), int(q.y));
		float k = mix(1.0 - w.x, w.x, o.x) * mix(1.0 - w.y, w.y, o.y) * c.a;
		sum += c.rgb * k;
		wsum += k;
	}
	vec4 nearest = psxTexel(tpage, clut, int(floor(t.x)), int(floor(t.y)));
	return vec4((wsum > 1e-4) ? sum / wsum : nearest.rgb, nearest.a);
}

// a polygon's texel range from its corner uvs: the largest is the edge the
// PS1 stops short of, unless it ends a power-of-two block (63, 127: inclusive)
void uvBounds(vec2 a, vec2 b, vec2 c, out vec2 lo, out vec2 hi)
{
	lo = min(a, min(b, c));
	hi = max(a, max(b, c));
	bvec2 inclusive = equal(mod(hi + 1.0, 8.0), vec2(0.0));
	hi = max(lo, vec2(inclusive.x ? hi.x : hi.x - 1.0, inclusive.y ? hi.y : hi.y - 1.0));
}

uint hashU(uint x)
{
	x ^= x >> 16;
	x *= 0x7feb352du;
	x ^= x >> 15;
	x *= 0x846ca68bu;
	x ^= x >> 16;
	return x;
}

uint byteAt(uint word, uint b) { return (word >> (b * 8u)) & 255u; }

vec3 levelPos(uint v)
{
	return vec3(uintBitsToFloat(levelVerts[v * 4u]), uintBitsToFloat(levelVerts[v * 4u + 1u]), uintBitsToFloat(levelVerts[v * 4u + 2u]));
}

vec3 levelCol(uint v)
{
	uint c = levelVerts[v * 4u + 3u];
	return vec3(float(c & 255u), float((c >> 8) & 255u), float((c >> 16) & 255u)) / 128.0;
}

// captured triangles (models, or the level as drawn): same 16-word layout
uint dynWord(uint inst, uint tri, uint w) { return (inst == INST_CAP) ? capTris[tri * 16u + w] : dynTris[tri * 16u + w]; }
uint dynByte(uint inst, uint tri, uint off) { return byteAt(dynWord(inst, tri, off >> 2), off & 3u); }

vec3 dynPos(uint inst, uint tri, uint c)
{
	return vec3(uintBitsToFloat(dynWord(inst, tri, c * 3u)), uintBitsToFloat(dynWord(inst, tri, c * 3u + 1u)), uintBitsToFloat(dynWord(inst, tri, c * 3u + 2u)));
}

// everything about a triangle at barycentrics
struct TriData
{
	vec3 p0, p1, p2;
	vec3 nSmooth; // interpolated vertex normal (unnormalized orientation)
	vec3 n0, n1, n2; // vertex normals (unnormalized, any sign; zero when unknown)
	vec4 texel;
	vec3 vcol;    // vertex colour / 0x80
	vec4 tint;    // level: the art's tint (fitArt), w glow
	uint flags;
	uint blend;   // 3 = opaque, else PS1 semi-transparency mode (0 avg, 1 add, 2 sub)
};

TriData fetchTriangle(uint inst, uint prim, vec2 bary)
{
	TriData d;
	vec3 w = vec3(1.0 - bary.x - bary.y, bary.x, bary.y);
	vec2 uv[3];
	vec3 col[3];
	uint clut = 0u, tpage = 0u;

	if (inst == INST_LEVEL)
	{
		uint w0 = levelTris[prim * 3u], w1 = levelTris[prim * 3u + 1u], w2 = levelTris[prim * 3u + 2u];
		uint v[3] = uint[3](w0 & 0xffffu, w0 >> 16, w1 & 0xffffu);
		uint slot = w1 >> 16;
		d.flags = w2 >> 24;
		d.p0 = levelPos(v[0]);
		d.p1 = levelPos(v[1]);
		d.p2 = levelPos(v[2]);
		d.n0 = levelNormals[F.extra2.x + prim * 3u].xyz;
		d.n1 = levelNormals[F.extra2.x + prim * 3u + 1u].xyz;
		d.n2 = levelNormals[F.extra2.x + prim * 3u + 2u].xyz;
		d.nSmooth = d.n0 * w.x + d.n1 * w.y + d.n2 * w.z;
		uint s0 = 0u, s1 = 0u;
		if (slot != 0xffffu)
		{
			s0 = texSlots[slot * 3u];
			s1 = texSlots[slot * 3u + 1u];
			uint s2 = texSlots[slot * 3u + 2u];
			clut = s2 & 0xffffu;
			tpage = s2 >> 16;
		}
		else
		{
			d.flags &= ~TRI_TEXTURED;
		}
		for (int i = 0; i < 3; i++)
		{
			uint corner = byteAt(w2, uint(i));
			uint word = (corner < 2u) ? s0 : s1;
			uint b = (corner & 1u) * 2u;
			uv[i] = vec2(float(byteAt(word, b)), float(byteAt(word, b + 1u)));
			col[i] = levelCol(v[i]);
		}
		d.tint = levelTint[v[0]] * w.x + levelTint[v[1]] * w.y + levelTint[v[2]] * w.z;
		// overlay 226: textures whose blend bits are not 3 draw semi-transparent; water always
		d.blend = ((d.flags & TRI_TEXTURED) != 0u) ? ((tpage >> 5) & 3u) : 3u;
		if ((d.flags & TRI_WATER) != 0u && d.blend == 3u)
		{
			d.blend = 0u;
		}
	}
	else
	{
		d.p0 = dynPos(inst, prim, 0u);
		d.p1 = dynPos(inst, prim, 1u);
		d.p2 = dynPos(inst, prim, 2u);
		d.n0 = (inst == INST_DYN) ? dynNormals[prim * 3u].xyz : vec3(0.0);
		d.n1 = (inst == INST_DYN) ? dynNormals[prim * 3u + 1u].xyz : vec3(0.0);
		d.n2 = (inst == INST_DYN) ? dynNormals[prim * 3u + 2u].xyz : vec3(0.0);
		d.nSmooth = d.n0 * w.x + d.n1 * w.y + d.n2 * w.z;
		for (uint i = 0u; i < 3u; i++)
		{
			uv[i] = vec2(float(dynByte(inst, prim, 36u + i * 2u)), float(dynByte(inst, prim, 37u + i * 2u)));
			col[i] = vec3(float(dynByte(inst, prim, 46u + i * 4u)), float(dynByte(inst, prim, 47u + i * 4u)), float(dynByte(inst, prim, 48u + i * 4u))) / 128.0;
		}
		clut = dynWord(inst, prim, 10u) >> 16;
		tpage = dynWord(inst, prim, 11u) & 0xffffu;
		d.flags = dynWord(inst, prim, 14u) >> 16;
		// the level as drawn keeps its see-through polygons; models are captured opaque
		// (wheels' colour-keyed sprites count as opaque)
		d.blend = ((inst == INST_CAP) && ((d.flags & TRI_SEMI) != 0u)) ? ((tpage >> 5) & 3u) : 3u;
		d.tint = vec4(1.0, 1.0, 1.0, 0.0);
	}

	d.vcol = col[0] * w.x + col[1] * w.y + col[2] * w.z;
	if ((d.flags & TRI_TEXTURED) != 0u)
	{
		vec2 t = uv[0] * w.x + uv[1] * w.y + uv[2] * w.z;
		if ((inst == INST_LEVEL) && ((d.flags & TRI_WATER) != 0u))
		{
			// water: the level's env map, tiled over the surface and drifting
			uint s0 = texSlots[(levelTris[prim * 3u + 1u] >> 16) * 3u], s1 = texSlots[(levelTris[prim * 3u + 1u] >> 16) * 3u + 1u];
			vec2 tl = vec2(float(byteAt(s0, 0u)), float(byteAt(s0, 1u)));
			vec2 br = vec2(float(byteAt(s1, 2u)), float(byteAt(s1, 3u)));
			vec3 pos = d.p0 * w.x + d.p1 * w.y + d.p2 * w.z;
			vec2 f = fract(pos.xz / 768.0 + vec2(0.021, 0.013) * F.params.y);
			t = mix(tl, br, f);
		}
		if (g_texFilter && (F.feat.x != 0u))
		{
			vec2 lo, hi;
			uvBounds(uv[0], uv[1], uv[2], lo, hi);
			d.texel = psxTexelFiltered(tpage, clut, t, lo, hi);
		}
		else
		{
			d.texel = psxTexel(tpage, clut, int(floor(t.x)), int(floor(t.y)));
		}
	}
	else
	{
		d.texel = vec4(0.5, 0.5, 0.5, 1.0); // untextured: vertex colour (x2 below) is the colour
	}
	return d;
}

// ray candidate: colour key 0x0000 never hits; semi-transparent surfaces only
// for rays that ask for them (camera rays), never for shadows and bounces
// (lean: runs for every triangle a ray meets, so it reads only what it needs)
bool candidateAccept(uint inst, uint prim, vec2 bary, bool includeSemi)
{
	uint flags, blend, tpage = 0u, clut = 0u;
	vec2 t = vec2(0.0);
	vec3 w = vec3(1.0 - bary.x - bary.y, bary.x, bary.y);
	if (inst == INST_LEVEL)
	{
		uint w1 = levelTris[prim * 3u + 1u], w2 = levelTris[prim * 3u + 2u];
		uint slot = w1 >> 16;
		flags = w2 >> 24;
		if (slot == 0xffffu)
		{
			flags &= ~TRI_TEXTURED;
		}
		if ((flags & TRI_TEXTURED) != 0u)
		{
			uint s0 = texSlots[slot * 3u], s1 = texSlots[slot * 3u + 1u], s2 = texSlots[slot * 3u + 2u];
			clut = s2 & 0xffffu;
			tpage = s2 >> 16;
			for (int i = 0; i < 3; i++)
			{
				uint corner = byteAt(w2, uint(i));
				uint word = (corner < 2u) ? s0 : s1;
				uint b = (corner & 1u) * 2u;
				t += w[i] * vec2(float(byteAt(word, b)), float(byteAt(word, b + 1u)));
			}
		}
		blend = ((flags & TRI_TEXTURED) != 0u) ? ((tpage >> 5) & 3u) : 3u;
		blend = (((flags & TRI_WATER) != 0u) && (blend == 3u)) ? 0u : blend;
		if ((flags & TRI_WATER) != 0u)
		{
			return includeSemi; // water: its env map is never colour keyed
		}
	}
	else
	{
		flags = dynWord(inst, prim, 14u) >> 16;
		clut = dynWord(inst, prim, 10u) >> 16;
		tpage = dynWord(inst, prim, 11u) & 0xffffu;
		blend = ((inst == INST_CAP) && ((flags & TRI_SEMI) != 0u)) ? ((tpage >> 5) & 3u) : 3u;
		if ((flags & TRI_TEXTURED) != 0u)
		{
			for (uint i = 0u; i < 3u; i++)
			{
				t += w[i] * vec2(float(dynByte(inst, prim, 36u + i * 2u)), float(dynByte(inst, prim, 37u + i * 2u)));
			}
		}
	}
	if (((flags & TRI_SMOKE) != 0u) || (!includeSemi && (blend != 3u)))
	{
		return false; // smoke is drawn by GL and only shades (smokeBlocks)
	}
	// colour key
	return ((flags & TRI_TEXTURED) == 0u) || (psxTexel(tpage, clut, int(floor(t.x)), int(floor(t.y))).a >= 0.5);
}

// smoke stops a shadow ray only partly: dark smoke ~60%, light ~30% (stochastic)
bool smokeBlocks(uint inst, uint prim, vec2 bary, vec3 o, vec3 d)
{
	if (inst != INST_DYN)
	{
		return false;
	}
	TriData t = fetchTriangle(inst, prim, bary);
	if (((t.flags & TRI_SMOKE) == 0u) || (t.texel.a < 0.5))
	{
		return false;
	}
	float opacity = (luma(t.texel.rgb * t.vcol) < 0.35) ? 0.6 : 0.3;
	uint h = hashU(prim * 9781u ^ floatBitsToUint(o.x + o.z * 0.37) ^ floatBitsToUint(d.y) ^ (F.size.z * 7919u));
	return float(h) * (1.0 / 4294967296.0) < opacity;
}

// Coincident polygons (decals: eyes and markings on models, floor markings,
// overlays in the level) hit at the same distance. The PS1 settles them by draw
// order; picking either would shimmer (z-fighting). Ties go to the polygon the
// PS1 draws on top: in the level as drawn (captured in OT walk order) the later
// one; in a model the earlier emitted one (same OT bucket: drawn last); in the
// level mesh, deterministically the later one.
vec3 triFaceNormal(uint inst, uint prim)
{
	vec3 p0, p1, p2;
	if (inst == INST_LEVEL)
	{
		uint w0 = levelTris[prim * 3u], w1 = levelTris[prim * 3u + 1u];
		p0 = levelPos(w0 & 0xffffu);
		p1 = levelPos(w0 >> 16);
		p2 = levelPos(w1 & 0xffffu);
	}
	else
	{
		p0 = dynPos(inst, prim, 0u);
		p1 = dynPos(inst, prim, 1u);
		p2 = dynPos(inst, prim, 2u);
	}
	vec3 n = cross(p1 - p0, p2 - p0);
	return n * inversesqrt(max(dot(n, n), 1e-20));
}

bool tieWins(uint inst, uint candPrim, float candT, uint comInst, uint comPrim, float comT)
{
	float eps = 0.02 + 1e-3 * comT;
	// decals a little off the surface they sit on (boosters, road markings):
	// parallel and a few units apart along the ray, the PS1 draws them over it
	// by order, not depth; nearest-wins would flicker between the two from
	// pixel to pixel and frame to frame. Same rule within a wider margin there
	float wide = 2.0 + 4e-3 * comT;
	float dt = abs(candT - comT);
	// boosters (animated textures; the texture layer's flagged by native_rtx.c)
	// cut through the road triangles they're set into, a few units above and
	// below: the animated one always wins against the floor around it
	if ((inst == comInst) && (inst != INST_DYN) && (dt < 8.0 + 0.01 * comT))
	{
		bool ca = (((inst == INST_LEVEL) ? (levelTris[candPrim * 3u + 2u] >> 24) : (capTris[candPrim * 16u + 14u] >> 16)) & TRI_ANIMATED) != 0u;
		bool ma = (((inst == INST_LEVEL) ? (levelTris[comPrim * 3u + 2u] >> 24) : (capTris[comPrim * 16u + 14u] >> 16)) & TRI_ANIMATED) != 0u;
		if ((ca != ma) && (abs(dot(triFaceNormal(inst, candPrim), triFaceNormal(comInst, comPrim))) > 0.8))
		{
			return ca;
		}
	}
	if ((inst == comInst) && (inst != INST_DYN) && (dt > eps) && (dt < wide) &&
	    (abs(dot(triFaceNormal(inst, candPrim), triFaceNormal(comInst, comPrim))) > 0.995))
	{
		return candPrim > comPrim;
	}
	if (candT > comT + eps)
	{
		return false;
	}
	if ((candT < comT - eps) || (inst != comInst))
	{
		return candT <= comT;
	}
	return (inst == INST_DYN) ? (candPrim < comPrim) : (candPrim > comPrim);
}

// a candidate the ray query offers: closer, or the tie winner, than what it holds
bool candidateBeats(rayQueryEXT rq, uint ci, uint cp)
{
	if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionTriangleEXT)
	{
		return true;
	}
	return tieWins(ci, cp, rayQueryGetIntersectionTEXT(rq, false), uint(rayQueryGetIntersectionInstanceCustomIndexEXT(rq, true)),
	               uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, true)), rayQueryGetIntersectionTEXT(rq, true));
}

// set when a ray of this invocation was blocked by / landed on a model: its
// light depends on things that move (trace.comp keeps only a short history)
bool g_touchedDyn = false;

bool triAnimated(uint inst, uint prim)
{
	return (((inst == INST_LEVEL) ? (levelTris[prim * 3u + 2u] >> 24) : (capTris[prim * 16u + 14u] >> 16)) & TRI_ANIMATED) != 0u;
}

bool capturedTexelShows(uint prim, vec2 bary, bool semi);

// a booster (animated texture) within margin of a hit at t along the ray, level
// mesh (INST_LEVEL) or texture layer (INST_CAP). Boosters cut through the road
// they're set into, partly below it, and the RT hardware doesn't offer what lies
// past a committed hit: so they're looked for explicitly, and always win
float traceAnimatedNear(uint inst, vec3 o, vec3 d, float t, float margin, bool semi, out uint prim, out vec2 bary)
{
	prim = 0u;
	bary = vec2(0.0);
	rayQueryEXT rq;
	rayQueryInitializeEXT(rq, tlas, gl_RayFlagsNoneEXT, (inst == INST_LEVEL) ? 0x1u : 0x4u, o, max(t - margin, 0.0), d, t + margin);
	while (rayQueryProceedEXT(rq))
	{
		if (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
		{
			uint cp = uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, false));
			vec2 cb = rayQueryGetIntersectionBarycentricsEXT(rq, false);
			if (triAnimated(inst, cp) && ((inst == INST_LEVEL) ? candidateAccept(inst, cp, cb, true) : capturedTexelShows(cp, cb, semi)))
			{
				rayQueryConfirmIntersectionEXT(rq);
			}
		}
	}
	if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionTriangleEXT)
	{
		return -1.0;
	}
	prim = uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, true));
	bary = rayQueryGetIntersectionBarycentricsEXT(rq, true);
	return rayQueryGetIntersectionTEXT(rq, true);
}

float boosterMargin(float t)
{
	return 8.0 + 0.01 * t;
}

// closest hit; returns t < 0 on miss
float traceClosest(vec3 o, vec3 d, float tmax, bool includeSemi, out uint inst, out uint prim, out vec2 bary)
{
	rayQueryEXT rq;
	rayQueryInitializeEXT(rq, tlas, (F.params.w > 0.5) && (F.params.w < 1.5) ? gl_RayFlagsOpaqueEXT : gl_RayFlagsNoneEXT, g_fastRays ? MASK_FAST : MASK_SCENE, o, 0.0, d, tmax);
	while (rayQueryProceedEXT(rq))
	{
		if (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
		{
			uint ci = uint(rayQueryGetIntersectionInstanceCustomIndexEXT(rq, false));
			uint cp = uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, false));
			if (candidateBeats(rq, ci, cp) && candidateAccept(ci, cp, rayQueryGetIntersectionBarycentricsEXT(rq, false), includeSemi))
			{
				rayQueryConfirmIntersectionEXT(rq);
			}
		}
	}
	inst = 0u;
	prim = 0u;
	bary = vec2(0.0);
	if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionTriangleEXT)
	{
		return -1.0;
	}
	inst = uint(rayQueryGetIntersectionInstanceCustomIndexEXT(rq, true));
	prim = uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, true));
	bary = rayQueryGetIntersectionBarycentricsEXT(rq, true);
	g_touchedDyn = g_touchedDyn || (inst == INST_DYN);
	float t = rayQueryGetIntersectionTEXT(rq, true);
	if (includeSemi && (inst == INST_LEVEL) && !triAnimated(INST_LEVEL, prim))
	{
		// (camera rays) a booster set into this floor wins
		uint bp;
		vec2 bb;
		float bt = traceAnimatedNear(INST_LEVEL, o, d, t, boosterMargin(t), false, bp, bb);
		if ((bt >= 0.0) && (abs(dot(triFaceNormal(INST_LEVEL, bp), triFaceNormal(INST_LEVEL, prim))) > 0.8))
		{
			prim = bp;
			bary = bb;
			t = bt;
		}
	}
	return t;
}

// Shadow rays from the level ignore the floor continuing: a nearly coplanar
// level triangle met close along the surface (a shallow fold between two floor
// pieces under a grazing sun). Those would shade one piece hard and not its
// neighbour; walls, overhangs, models and anything standing clear above the
// surface still cast shadows. Set by lightSample around its shadow rays.
// set by lightSample (level surfaces): see facesLight
bool g_skipFold = false;
vec3 g_foldN = vec3(0.0);

bool foldOccluder(uint ci, uint cp, vec2 bary, vec3 o, vec3 d, float t)
{
	if (!g_skipFold || (ci != INST_LEVEL) || (t > 600.0))
	{
		return false;
	}
	TriData h = fetchTriangle(ci, cp, bary);
	vec3 n = normalize(cross(h.p1 - h.p0, h.p2 - h.p0));
	float rise = dot(d * t, g_foldN); // how far above the origin's plane the hit is
	float facing = abs(dot(n, g_foldN));
	// also the neighbouring faces of the same smooth surface (within the ~70 degrees
	// weldLevelNormals smooths across: ridges, dune folds) met close and low: their
	// smooth normals already turn the light off softly, the flat faces would cut
	// it hard at every polygon edge
	return ((facing > 0.9) && (rise < max(20.0, 0.2 * t))) || ((facing > 0.34) && (t < 250.0) && (rise < max(30.0, 0.35 * t)));
}

// set by shadowRay for rays that leave a level surface below its own flat
// triangle (lit only by its smooth normal, see facesLight): such a ray runs
// under the terrain, where the PS1 culls the single-sided polygons it meets from
// behind. They don't block it; or every fold would shade hard (a dark band
// beyond each ridge, a bright line where the ray offset just clears it)
bool g_cullBack = false;

bool levelBackFacing(uint prim, vec3 d)
{
	if ((F.params.z == 0.0) || (((levelTris[prim * 3u + 2u] >> 24) & TRI_DOUBLE_SIDED) != 0u))
	{
		return false;
	}
	// the corner normals point to the oriented front (ctr_rtx.c levelOrient)
	uint c = F.extra2.x + prim * 3u;
	return (levelNormals[c].w > 0.5) && dot(levelNormals[c].xyz + levelNormals[c + 1u].xyz + levelNormals[c + 2u].xyz, d) > 0.0;
}

bool occludedMask(vec3 o, vec3 d, float tmax, uint mask)
{
	rayQueryEXT rq;
	mask = (g_fastRays && (mask == MASK_SCENE)) ? MASK_FAST : mask;
	rayQueryInitializeEXT(rq, tlas, gl_RayFlagsTerminateOnFirstHitEXT | (((F.params.w > 0.5) && (F.params.w < 1.5)) ? gl_RayFlagsOpaqueEXT : 0u), mask, o, 0.0, d, tmax);
	while (rayQueryProceedEXT(rq))
	{
		if (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
		{
			uint ci = uint(rayQueryGetIntersectionInstanceCustomIndexEXT(rq, false));
			uint cp = uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, false));
			vec2 cb = rayQueryGetIntersectionBarycentricsEXT(rq, false);
			if (g_cullBack && (ci == INST_LEVEL) && levelBackFacing(cp, d))
			{
				continue;
			}
			if ((candidateAccept(ci, cp, cb, false) && !foldOccluder(ci, cp, cb, o, d, rayQueryGetIntersectionTEXT(rq, false))) || smokeBlocks(ci, cp, cb, o, d))
			{
				rayQueryConfirmIntersectionEXT(rq);
			}
		}
	}
	bool hit = rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
	g_touchedDyn = g_touchedDyn || (hit && (uint(rayQueryGetIntersectionInstanceCustomIndexEXT(rq, true)) == INST_DYN));
	return hit;
}

bool occluded(vec3 o, vec3 d, float tmax) { return occludedMask(o, d, tmax, MASK_SCENE); }

// a texture layer polygon's texel at bary is drawn (not colour keyed) and it is
// see-through as asked (lean: reads only what it needs, as candidateAccept)
bool capturedTexelShows(uint prim, vec2 bary, bool semi)
{
	uint flags = capTris[prim * 16u + 14u] >> 16;
	uint clut = capTris[prim * 16u + 10u] >> 16, tpage = capTris[prim * 16u + 11u] & 0xffffu;
	uint blend = ((flags & TRI_SEMI) != 0u) ? ((tpage >> 5) & 3u) : 3u;
	if (semi != (blend != 3u))
	{
		return false;
	}
	if ((flags & TRI_TEXTURED) == 0u)
	{
		return true;
	}
	vec3 w = vec3(1.0 - bary.x - bary.y, bary.x, bary.y);
	vec2 t = vec2(0.0);
	for (uint i = 0u; i < 3u; i++)
	{
		t += w[i] * vec2(float(dynByte(INST_CAP, prim, 36u + i * 2u)), float(dynByte(INST_CAP, prim, 37u + i * 2u)));
	}
	return psxTexel(tpage, clut, int(floor(t.x)), int(floor(t.y))).a >= 0.5;
}

// the PS1's own polygon for the surface a primary ray hit at t (texture layer):
// same place within eps, alpha-keyed like the PS1
bool traceCaptured(vec3 o, vec3 d, float t, float eps, bool semi, out uint prim, out vec2 bary)
{
	// a booster as the PS1 drew it wins (traceAnimatedNear)
	if (traceAnimatedNear(INST_CAP, o, d, t, max(eps, boosterMargin(t)), semi, prim, bary) >= 0.0)
	{
		return true;
	}
	prim = 0u;
	bary = vec2(0.0);
	rayQueryEXT rq;
	rayQueryInitializeEXT(rq, tlas, gl_RayFlagsNoneEXT, 0x4u, o, max(t - eps, 0.0), d, t + eps);
	while (rayQueryProceedEXT(rq))
	{
		if (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
		{
			uint cp = uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, false));
			if (capturedTexelShows(cp, rayQueryGetIntersectionBarycentricsEXT(rq, false), semi) && candidateBeats(rq, INST_CAP, cp))
			{
				rayQueryConfirmIntersectionEXT(rq);
			}
		}
	}
	if (rayQueryGetIntersectionTypeEXT(rq, true) != gl_RayQueryCommittedIntersectionTriangleEXT)
	{
		return false;
	}
	prim = uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, true));
	bary = rayQueryGetIntersectionBarycentricsEXT(rq, true);
	return true;
}

struct Surface
{
	vec3 pos;
	vec3 geoNormal; // facing the incoming ray: ray offsets
	vec3 normal;    // smooth shading normal, same side
	vec3 shadowPos; // where rays leave from: off the flat triangle like the smooth surface it stands for
	vec3 albedo;    // linear
	vec3 ps1Color;  // texel x vertex colour as the PS1 draws it (display space, 0..2)
	vec3 ps1Light;  // linear vertex-colour light (hybrid): albedo x ps1Light = the PS1 colour
	uint flags;
	uint blend;
	bool dynamic;
	vec4 tint; // level: the art's tint, w glow
};

vec3 srgbToLinear(vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }
vec3 linearToSrgb(vec3 c) { return pow(max(c, vec3(0.0)), vec3(1.0 / 2.2)); }

void surfaceMaterial(inout Surface s, TriData d, uint inst);

Surface surfaceAt(uint inst, uint prim, vec2 bary, vec3 rayDir)
{
	Surface s;
	TriData d = fetchTriangle(inst, prim, bary);
	vec3 w = vec3(1.0 - bary.x - bary.y, bary.x, bary.y);
	s.pos = d.p0 * w.x + d.p1 * w.y + d.p2 * w.z;
	vec3 n = normalize(cross(d.p1 - d.p0, d.p2 - d.p0));
	s.geoNormal = (dot(n, rayDir) > 0.0) ? -n : n;
	vec3 ns = (dot(d.nSmooth, d.nSmooth) > 1e-8) ? normalize(d.nSmooth) : n;
	ns = (dot(ns, s.geoNormal) < 0.0) ? -ns : ns;
	// keep the smooth normal within 90 degrees of the view side
	s.normal = normalize(mix(s.geoNormal, ns, clamp(dot(ns, s.geoNormal) * 4.0, 0.0, 1.0)));
	// shadow terminator (Hanika 2021): low-poly meshes shaded with smooth normals
	// self-shadow in dark bands along their edges when rays leave the flat
	// triangle; start them from the curved surface the vertex normals describe
	s.shadowPos = s.pos;
	if ((dot(d.n0, d.n0) > 1e-12) && (dot(d.n1, d.n1) > 1e-12) && (dot(d.n2, d.n2) > 1e-12))
	{
		vec3 pv[3] = vec3[3](d.p0, d.p1, d.p2);
		vec3 nv[3] = vec3[3](d.n0, d.n1, d.n2);
		vec3 acc = vec3(0.0);
		for (int i = 0; i < 3; i++)
		{
			vec3 ni = normalize(nv[i]);
			ni = (dot(ni, s.geoNormal) < 0.0) ? -ni : ni;
			acc += w[i] * (s.pos - min(dot(s.pos - pv[i], ni), 0.0) * ni);
		}
		s.shadowPos = (dot(acc - s.pos, s.geoNormal) > 0.0) ? acc : s.pos;
	}
	s.flags = d.flags;
	s.blend = d.blend;
	s.dynamic = (inst == INST_DYN);
	s.tint = d.tint;
	surfaceMaterial(s, d, inst);
	return s;
}

// what the texture and vertex colour make of a surface (also from the texture layer)
void surfaceMaterial(inout Surface s, TriData d, uint inst)
{
	// PS1 modulation is texel * vertex colour / 0x80. Vertex colours are the
	// artists' light (level) or the GTE's (models): the albedo is the texel, the
	// light is traced.
	bool textured = (d.flags & TRI_TEXTURED) != 0u;
	// level: the art's colour that its light doesn't explain (fitArt) stays in the surface
	vec3 tint = (inst == INST_LEVEL) ? s.tint.rgb : vec3(1.0);
	if (inst == INST_DYN)
	{
		// models: vertex colour = material colour x the GTE's light. Keep its
		// colour fully and a little of its brightness (the rest is traced)
		float lv = max(luma(d.vcol), 0.02);
		tint = srgbToLinear(clamp(d.vcol / lv, 0.25, 4.0) * pow(lv, 0.35));
	}
	s.albedo = textured ? min(srgbToLinear(d.texel.rgb) * tint, vec3(0.95)) : min(srgbToLinear(clamp(d.vcol * 0.5, 0.0, 1.0)), vec3(0.95));
	s.ps1Color = textured ? d.texel.rgb * d.vcol : clamp(d.vcol * 0.5, 0.0, 1.0);
	// the PS1 light in the same units as the traced light (linear vertex colour):
	// hybrid mode's base, the emission of glowing surfaces
	s.ps1Light = textured ? srgbToLinear(d.vcol) : srgbToLinear(clamp(d.vcol * 0.5, 0.0, 1.0)) / max(s.albedo, vec3(0.05));
	if (F.extra.z == 0u)
	{
		s.albedo = textured ? min(srgbToLinear(d.texel.rgb), vec3(0.95)) : vec3(0.95);
		s.ps1Light = srgbToLinear(textured ? d.vcol : clamp(d.vcol * 0.5, 0.0, 1.0)) / (textured ? 1.0 : 0.95);
	}
}

// ---- random numbers --------------------------------------------------------

uint pcg(inout uint state)
{
	state = state * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float rnd(inout uint state) { return float(pcg(state)) * (1.0 / 4294967296.0); }

// two blue noise numbers for pixel px's sample s of dimension pair dim: the tile
// moved every frame (R2 sequence: evenly spread over the frames), each sample
// rotated by the golden ratio, each dimension pair from elsewhere in the tile
vec2 blueNoise2(uvec2 px, uint s, uint dim)
{
	uvec2 off = uvec2(fract(vec2(0.7548777, 0.5698403) * float(F.size.z & 4095u)) * 64.0) + dim * uvec2(23u, 41u);
	uvec2 p = (px + off) & 63u, p2 = (p + uvec2(32u, 17u)) & 63u;
	vec2 v = vec2(blueNoise[p.y * 64u + p.x], blueNoise[p2.y * 64u + p2.x]);
	return fract(v + float(s) * vec2(0.6180340, 0.7548777));
}

// set around the camera hit's direct light (trace.comp): the sun's direction in
// its cone from blue noise
bool g_useBn = false;
vec2 g_bnSun = vec2(0.0);

vec3 cosineSampleU(vec3 n, vec2 u)
{
	float u1 = u.x, u2 = u.y;
	float r = sqrt(u1), phi = 6.28318530718 * u2;
	vec3 t = normalize(abs(n.y) < 0.99 ? cross(n, vec3(0, 1, 0)) : cross(n, vec3(1, 0, 0)));
	vec3 b = cross(n, t);
	return normalize(t * (r * cos(phi)) + b * (r * sin(phi)) + n * sqrt(max(0.0, 1.0 - u1)));
}

vec3 cosineSample(vec3 n, inout uint rng)
{
	float u1 = rnd(rng);
	return cosineSampleU(n, vec2(u1, rnd(rng)));
}

vec3 coneSampleU(vec3 d, float angle, vec2 u)
{
	float cosMax = cos(angle);
	float z = mix(cosMax, 1.0, u.x);
	float r = sqrt(max(0.0, 1.0 - z * z));
	float phi = 6.28318530718 * u.y;
	vec3 t = normalize(abs(d.y) < 0.99 ? cross(d, vec3(0, 1, 0)) : cross(d, vec3(1, 0, 0)));
	vec3 b = cross(d, t);
	return normalize(t * (r * cos(phi)) + b * (r * sin(phi)) + d * z);
}

vec3 coneSample(vec3 d, float angle, inout uint rng)
{
	float u1 = rnd(rng);
	return coneSampleU(d, angle, vec2(u1, rnd(rng)));
}

vec3 skyRadiance(vec3 d)
{
	if (F.ambient.w > 0.5)
	{
		return F.ambient.rgb;
	}
	if (F.gridDim.w != 0u)
	{
		// the level's own sky colours (already linear and scaled), bilinear
		float el = asin(clamp(d.y, -1.0, 1.0));
		float r = clamp((0.5 - el / 3.14159265) * 8.0 - 0.5, 0.0, 7.0);
		float c = (atan(d.z, d.x) / 6.2831853 + 0.5) * 8.0 - 0.5;
		int r0 = int(floor(r)), r1 = min(r0 + 1, 7);
		int c0 = int(floor(c)), c1 = c0 + 1;
		float fr = r - float(r0), fc = c - floor(c);
		c0 = (c0 + 8) & 7;
		c1 = (c1 + 8) & 7;
		vec3 top = mix(F.sky[r0 * 8 + c0].rgb, F.sky[r0 * 8 + c1].rgb, fc);
		vec3 bot = mix(F.sky[r1 * 8 + c0].rgb, F.sky[r1 * 8 + c1].rgb, fc);
		return mix(top, bot, fr);
	}
	if (d.y < 0.0)
	{
		return F.groundColor.rgb;
	}
	return mix(F.skyHorizon.rgb, F.skyZenith.rgb, sqrt(d.y));
}

float rayEps(vec3 p)
{
	return F.skyHorizon.w + 1e-4 * length(p - F.camPos.xyz);
}

// low-poly models self-shadow along their facets: push their rays further out
float surfaceEps(vec3 p, bool dynamic)
{
	return rayEps(p) * (dynamic ? 3.0 : 1.0);
}

// The level is shaded smooth (smooth normals), like the PS1's Gouraud vertex
// colours: whether a light reaches it is decided by that normal, not by which
// way the flat triangle faces, or shading would cut off hard at every polygon
// edge of a dune or a slope. (Models keep the flat test.) Such rays start off
// the smooth side; the surface's own coplanar triangles don't block them
// (foldOccluder, the same switch: g_skipFold)
bool facesLight(vec3 ng, vec3 dir)
{
	return g_skipFold || (dot(ng, dir) > 0.0);
}

vec3 rayOrigin(vec3 p, vec3 n, vec3 ng, vec3 dir, float eps)
{
	return p + ((dot(ng, dir) > 0.0) ? ng : n) * eps;
}

// shadow ray from a surface towards a light (see g_cullBack)
bool shadowRay(vec3 p, vec3 n, vec3 ng, vec3 dir, float tmax, float eps)
{
	g_cullBack = g_skipFold && (dot(ng, dir) <= 0.0);
	bool hit = occluded(rayOrigin(p, n, ng, dir, eps), dir, tmax);
	g_cullBack = false;
	return hit;
}

// point light falloff: 1 / (1 + d^2/f^2), windowed to reach exactly zero at
// 5 f (the light grid's reach) so no light ever stops with a hard edge
float lightFalloff(float d2, float f)
{
	float x = d2 / (25.0 * f * f);
	float win = clamp(1.0 - x * x, 0.0, 1.0);
	return win * win / (1.0 + d2 / (f * f));
}

// one shadowed sample from lights[first, first+count): 16 candidates picked by
// their unshadowed contribution (resampled importance sampling), one shadow ray
vec3 sampleLightSet(uint first, uint count, vec3 p, vec3 n, vec3 ng, float eps, inout uint rng)
{
	if (count == 0u)
	{
		return vec3(0.0);
	}
	const int M = 16;
	float wsum = 0.0, chosenW = 0.0;
	uint chosen = first;
	for (int i = 0; i < M; i++)
	{
		uint j = first + pcg(rng) % count;
		vec4 l0 = lights[j * 3u], l1 = lights[j * 3u + 1u];
		vec3 toL = l0.xyz - p;
		float d2 = max(dot(toL, toL), 1.0);
		float ndl = max(dot(n, toL * inversesqrt(d2)), 0.0);
		float w = luma(l1.rgb) * ndl * lightFalloff(d2, l1.w);
		wsum += w;
		if (rnd(rng) * wsum < w)
		{
			chosen = j;
			chosenW = w;
		}
	}
	if (chosenW <= 0.0)
	{
		return vec3(0.0);
	}
	vec4 l0 = lights[chosen * 3u], l1 = lights[chosen * 3u + 1u], l2 = lights[chosen * 3u + 2u];
	float u = rnd(rng) * 2.0 - 1.0, phi = 6.28318530718 * rnd(rng);
	vec3 target = l0.xyz + vec3(sqrt(1.0 - u * u) * cos(phi), u, sqrt(1.0 - u * u) * sin(phi)) * l2.x;
	vec3 toL = target - p;
	float dist = length(toL);
	vec3 dir = toL / max(dist, 1e-3);
	float ndl = dot(n, dir);
	if ((ndl <= 0.0) || !facesLight(ng, dir) || shadowRay(p, n, ng, dir, dist - eps * 2.0, eps))
	{
		return vec3(0.0);
	}
	vec3 f = l1.rgb * ndl * lightFalloff(dist * dist, l1.w);
	// RIS estimate f/p_hat * mean(p_hat); the caller multiplies by count (uniform source pdf 1/count)
	return f / chosenW * (wsum / float(M));
}

// how much a static light would give p, unshadowed: what light picks are weighed by
float staticLightTarget(uint j, vec3 p, vec3 n)
{
	vec4 l0 = lights[j * 3u], l1 = lights[j * 3u + 1u];
	vec3 toL = l0.xyz - p;
	float d2 = max(dot(toL, toL), 1.0);
	float ndl = max(dot(n, toL * inversesqrt(d2)), 0.0);
	return luma(l1.rgb) * ndl * lightFalloff(d2, l1.w);
}

// the static lights reaching p's light grid cell: lightGrid[offset, offset+count)
bool staticLightCell(vec3 p, out uint offset, out uint count)
{
	offset = 0u;
	count = 0u;
	ivec3 c = ivec3(floor((p - F.gridMin.xyz) / F.gridMin.w));
	if (any(lessThan(c, ivec3(0))) || any(greaterThanEqual(c, ivec3(F.gridDim.xyz))))
	{
		return false;
	}
	uint cell = (uint(c.z) * F.gridDim.y + uint(c.y)) * F.gridDim.x + uint(c.x);
	offset = lightGrid[cell * 2u];
	count = lightGrid[cell * 2u + 1u];
	return count > 0u;
}

// ---- specular (GGX) ----------------------------------------------------------
// Direct highlights cost no rays: the sun and static light samples add their
// GGX share to g_specOut with the shadow ray their diffuse light already cast.
// trace.comp sets the material around the camera hit's light samples
// (g_specOn); everything else (bounce hits, reflections) is diffuse only.
bool g_specOn = false;
vec3 g_specV = vec3(0.0);   // towards the viewer
float g_specRough = 0.7;    // perceptual roughness
vec3 g_specF0 = vec3(0.04);
vec3 g_specOut = vec3(0.0);
vec3 g_specLast = vec3(0.0); // (shadeStaticLight -> sampleStaticLights, before its RIS weight)

vec3 fresnelSchlick(vec3 f0, float c)
{
	return f0 + (1.0 - f0) * pow(1.0 - clamp(c, 0.0, 1.0), 5.0);
}

// GGX x Smith (Schlick-GGX) x Fresnel / (4 n.l n.v), times n.l: the light's
// specular share
vec3 ggxSpecular(vec3 n, vec3 v, vec3 l, float rough, vec3 f0)
{
	float ndl = dot(n, l), ndv = dot(n, v);
	if ((ndl <= 0.0) || (ndv <= 0.0))
	{
		return vec3(0.0);
	}
	vec3 h = normalize(v + l);
	float ndh = max(dot(n, h), 0.0);
	float a = rough * rough, a2 = a * a;
	float dd = ndh * ndh * (a2 - 1.0) + 1.0;
	float D = a2 / (3.14159265 * dd * dd);
	float k = (rough + 1.0) * (rough + 1.0) / 8.0;
	float G = (ndl / (ndl * (1.0 - k) + k)) * (ndv / (ndv * (1.0 - k) + k));
	return D * G * fresnelSchlick(f0, dot(v, h)) / (4.0 * ndv);
}

// the reflected environment's share for a rough surface (Karis' analytic fit
// of the split-sum table)
vec3 envBrdfApprox(vec3 f0, float rough, float ndv)
{
	const vec4 c0 = vec4(-1.0, -0.0275, -0.572, 0.022);
	const vec4 c1 = vec4(1.0, 0.0425, 1.04, -0.04);
	vec4 r = rough * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * ndv)) * r.x + r.y;
	vec2 ab = vec2(-1.04, 1.04) * a004 + r.zw;
	return f0 * ab.x + ab.y;
}

// one static light's shadowed light at p (a random point on it); g_lightBlocked
// tells whether its shadow ray was blocked (ReSTIR doesn't carry such a pick on)
bool g_lightBlocked = false;

vec3 shadeStaticLight(uint j, vec3 p, vec3 n, vec3 ng, float eps, inout uint rng)
{
	g_lightBlocked = false;
	vec4 l0 = lights[j * 3u], l1 = lights[j * 3u + 1u], l2 = lights[j * 3u + 2u];
	float u = rnd(rng) * 2.0 - 1.0, phi = 6.28318530718 * rnd(rng);
	vec3 target = l0.xyz + vec3(sqrt(1.0 - u * u) * cos(phi), u, sqrt(1.0 - u * u) * sin(phi)) * l2.x;
	vec3 toL = target - p;
	float dist = length(toL);
	vec3 dir = toL / max(dist, 1e-3);
	float ndl = dot(n, dir);
	if ((ndl <= 0.0) || !facesLight(ng, dir))
	{
		return vec3(0.0);
	}
	if (shadowRay(p, n, ng, dir, dist - eps * 2.0, eps))
	{
		g_lightBlocked = true;
		return vec3(0.0);
	}
	vec3 radiance = l1.rgb * lightFalloff(dist * dist, l1.w);
	g_specLast = g_specOn ? radiance * ggxSpecular(n, g_specV, dir, g_specRough, g_specF0) : vec3(0.0);
	return radiance * ndl;
}

// Static lights at camera hits come from the pixel's reservoir (trace.comp:
// ReSTIR, its light pick carried over the frames): g_resMode 1, the light
// g_resLight (0 none, else index + 1) weighted g_resW. Elsewhere (bounce hits)
// g_risM candidates from the light grid, one picked by unshadowed contribution
uint g_resMode = 0u, g_resLight = 0u;
float g_resW = 0.0;
int g_risM = 16; // (28 before blue noise sampling and TAA)

vec3 sampleStaticLights(vec3 p, vec3 n, vec3 ng, float eps, inout uint rng)
{
	g_specLast = vec3(0.0);
	if (g_resMode == 1u)
	{
		vec3 r = (g_resLight == 0u) ? vec3(0.0) : shadeStaticLight(g_resLight - 1u, p, n, ng, eps, rng) * g_resW;
		g_specOut += g_specLast * g_resW;
		return r;
	}
	uint offset, count;
	if (!staticLightCell(p, offset, count))
	{
		return vec3(0.0);
	}
	float wsum = 0.0, chosenW = 0.0;
	uint chosen = 0u;
	for (int i = 0; i < g_risM; i++)
	{
		uint j = lightGrid[offset + pcg(rng) % count];
		float w = staticLightTarget(j, p, n);
		wsum += w;
		if (rnd(rng) * wsum < w)
		{
			chosen = j;
			chosenW = w;
		}
	}
	if (chosenW <= 0.0)
	{
		return vec3(0.0);
	}
	// RIS estimate f/p_hat * mean(p_hat) x count (uniform source pdf 1/count)
	float risW = (wsum / float(g_risM)) * float(count) / chosenW;
	vec3 r = shadeStaticLight(chosen, p, n, ng, eps, rng) * risW;
	g_specOut += g_specLast * risW;
	return r;
}

// light from this frame's emitters (flames, turbos, particles): traced and
// accumulated apart from the rest, so it doesn't smear behind moving karts
vec3 dynLightSample(vec3 p, vec3 n, vec3 ng, inout uint rng)
{
	uint numDyn = F.extra.x;
	return sampleLightSet(F.counts.x, numDyn, p, n, ng, rayEps(p), rng) * float(numDyn);
}

vec3 lightSampleAll(vec3 p, vec3 n, vec3 ng, bool dynamic, inout uint rng);

// direct light at a surface: sun (outdoors), static lights (the art's lamps and
// glowing surfaces)
vec3 lightSample(vec3 p, vec3 n, vec3 ng, bool dynamic, inout uint rng)
{
	vec3 result = lightSampleAll(p, n, ng, dynamic, rng);
	g_skipFold = false;
	return result;
}

vec3 lightSampleAll(vec3 p, vec3 n, vec3 ng, bool dynamic, inout uint rng)
{
	vec3 result = vec3(0.0);
	float eps = rayEps(p);
	g_skipFold = !dynamic;
	g_foldN = ng;
	if (F.extra.z == 0u)
	{
		// hybrid: the track's own lighting is the base (it already holds the
		// level's lamps and glows); models get the static lights too
		return dynamic ? sampleStaticLights(p, n, ng, eps, rng) : vec3(0.0);
	}

	if (F.ambient.w < 0.5)
	{
		vec3 l = g_useBn ? coneSampleU(F.sunDir.xyz, F.sunDir.w, g_bnSun) : coneSample(F.sunDir.xyz, F.sunDir.w, rng);
		float ndl = dot(n, l);
		if ((ndl > 0.0) && facesLight(ng, l) && !shadowRay(p, n, ng, l, 1e7, eps))
		{
			result += F.sunColor.rgb * ndl;
			g_specOut += g_specOn ? F.sunColor.rgb * ggxSpecular(n, g_specV, l, g_specRough, g_specF0) : vec3(0.0);
		}
	}
	if (abs(F.params.w - 5.0) > 0.5) // experiment 5: no static lights (timing)
	{
		result += sampleStaticLights(p, n, ng, eps, rng);
	}
	return result;
}

// ---- world-space radiance cache ---------------------------------------------
// Indirect light per cell (size grows with camera distance, one per facing
// axis), written from visible surfaces and read where bounce rays land: every
// frame adds another bounce, so light keeps spreading (colour bleeding).

// jitter (0..1 each) dithers the cell: lookups jitter position and size level so
// neighbouring cells and sizes blend (no hard cell edges or rings on floors);
// writes use none
void cacheCell(vec3 p, vec3 n, vec4 jitter, out uint slot, out uint key)
{
	float base = F.params2.y;
	float dist = length(p - F.camPos.xyz);
	float lf = log2(max(dist * 0.015 / base, 1.0));
	int level = clamp(int(floor(lf + jitter.w - 0.5)), 0, 12);
	float size = base * exp2(float(level));
	ivec3 c = ivec3(floor(p / size + jitter.xyz - 0.5));
	vec3 an = abs(n);
	uint axis = (an.x > an.y && an.x > an.z) ? ((n.x > 0.0) ? 0u : 1u) : ((an.y > an.z) ? ((n.y > 0.0) ? 2u : 3u) : ((n.z > 0.0) ? 4u : 5u));
	uint h = hashU(uint(c.x) + hashU(uint(c.y) + hashU(uint(c.z) + hashU(uint(level) * 8u + axis))));
	slot = h;
	key = hashU(h ^ 0x9e3779b9u) | 1u;
}

const uint CACHE_PROBES = 8u;

vec3 cacheLookup(vec3 p, vec3 n, vec3 fallback, inout uint rng)
{
	uint h, key;
	cacheCell(p, n, vec4(rnd(rng), rnd(rng), rnd(rng), rnd(rng)), h, key);
	uint mask = F.extra2.y - 1u;
	for (uint i = 0u; i < CACHE_PROBES; i++)
	{
		uint s = (h + i) & mask;
		if (cacheKeys[s * 2u] == key)
		{
			vec4 v = cacheVal[s];
			// young cells blend towards the fallback
			return mix(fallback, v.rgb, clamp(v.w / 4.0, 0.0, 1.0));
		}
	}
	return fallback;
}

void cacheAdd(vec3 p, vec3 n, vec3 value)
{
	uint h, key;
	cacheCell(p, n, vec4(0.5), h, key);
	uint mask = F.extra2.y - 1u;
	uvec3 q = uvec3(clamp(value, 0.0, 64.0) * 256.0 + 0.5);
	for (uint i = 0u; i < CACHE_PROBES; i++)
	{
		uint s = (h + i) & mask;
		uint prev = atomicCompSwap(cacheKeys[s * 2u], 0u, key);
		if ((prev == 0u) || (prev == key))
		{
			atomicAdd(cacheAccum[s * 4u], q.r);
			atomicAdd(cacheAccum[s * 4u + 1u], q.g);
			atomicAdd(cacheAccum[s * 4u + 2u], q.b);
			atomicAdd(cacheAccum[s * 4u + 3u], 1u);
			return;
		}
	}
}

// what bounce rays see that the cache doesn't know yet
vec3 indirectFallback()
{
	return ((F.ambient.w > 0.5) ? F.ambient.rgb : 0.5 * (F.skyZenith.rgb + F.skyHorizon.rgb)) * 0.5;
}
