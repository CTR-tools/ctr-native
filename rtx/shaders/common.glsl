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
	vec4 params;      // aoRadius, time (seconds), unused, unused
	uvec4 extra;      // numDynLights (stored after the static lights), unused...
} F;

layout(set = 0, binding = 2, std430) readonly buffer LevelVerts { uint levelVerts[]; };   // 4 words per vertex
layout(set = 0, binding = 3, std430) readonly buffer LevelTris { uint levelTris[]; };     // 3 words per triangle
layout(set = 0, binding = 4, std430) readonly buffer TexSlots { uint texSlots[]; };       // 3 words per slot
layout(set = 0, binding = 5, std430) readonly buffer DynTris { uint dynTris[]; };         // 16 words per triangle
layout(set = 0, binding = 6, std430) readonly buffer Vram { uint vram[]; };               // 1024x512 u16
layout(set = 0, binding = 7, std430) buffer Lights { vec4 lights[]; };                    // 3 vec4 per light
layout(set = 0, binding = 17, std430) readonly buffer LevelNormals { vec4 levelNormals[]; }; // smooth, per vertex
layout(set = 0, binding = 18, std430) readonly buffer DynNormals { vec4 dynNormals[]; };     // smooth, 3 per triangle

const uint INST_LEVEL = 0u;
const uint INST_DYN = 1u;

const uint TRI_TEXTURED = 1u;
const uint TRI_DOUBLE_SIDED = 2u;
const uint TRI_WATER = 4u;
const uint TRI_EMISSIVE = 8u;

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

uint dynWord(uint tri, uint w) { return dynTris[tri * 16u + w]; }
uint dynByte(uint tri, uint off) { return byteAt(dynWord(tri, off >> 2), off & 3u); }

vec3 dynPos(uint tri, uint c)
{
	return vec3(uintBitsToFloat(dynWord(tri, c * 3u)), uintBitsToFloat(dynWord(tri, c * 3u + 1u)), uintBitsToFloat(dynWord(tri, c * 3u + 2u)));
}

// everything about a triangle at barycentrics
struct TriData
{
	vec3 p0, p1, p2;
	vec3 nSmooth; // interpolated vertex normal (unnormalized orientation)
	vec4 texel;
	vec3 vcol;    // vertex colour / 0x80
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
		d.nSmooth = levelNormals[v[0]].xyz * w.x + levelNormals[v[1]].xyz * w.y + levelNormals[v[2]].xyz * w.z;
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
		// overlay 226: textures whose blend bits are not 3 draw semi-transparent; water always
		d.blend = ((d.flags & TRI_TEXTURED) != 0u) ? ((tpage >> 5) & 3u) : 3u;
		if ((d.flags & TRI_WATER) != 0u && d.blend == 3u)
		{
			d.blend = 0u;
		}
	}
	else
	{
		d.p0 = dynPos(prim, 0u);
		d.p1 = dynPos(prim, 1u);
		d.p2 = dynPos(prim, 2u);
		d.nSmooth = dynNormals[prim * 3u].xyz * w.x + dynNormals[prim * 3u + 1u].xyz * w.y + dynNormals[prim * 3u + 2u].xyz * w.z;
		for (uint i = 0u; i < 3u; i++)
		{
			uv[i] = vec2(float(dynByte(prim, 36u + i * 2u)), float(dynByte(prim, 37u + i * 2u)));
			col[i] = vec3(float(dynByte(prim, 46u + i * 4u)), float(dynByte(prim, 47u + i * 4u)), float(dynByte(prim, 48u + i * 4u))) / 128.0;
		}
		clut = dynWord(prim, 10u) >> 16;
		tpage = dynWord(prim, 11u) & 0xffffu;
		d.flags = dynWord(prim, 14u) >> 16;
		d.blend = 3u; // only opaque model polygons are captured
	}

	d.vcol = col[0] * w.x + col[1] * w.y + col[2] * w.z;
	if ((d.flags & TRI_TEXTURED) != 0u)
	{
		vec2 t = uv[0] * w.x + uv[1] * w.y + uv[2] * w.z;
		d.texel = psxTexel(tpage, clut, int(floor(t.x)), int(floor(t.y)));
	}
	else
	{
		d.texel = vec4(0.5, 0.5, 0.5, 1.0); // untextured: vertex colour (x2 below) is the colour
	}
	return d;
}

// ray candidate: colour key 0x0000 never hits; semi-transparent surfaces only
// for rays that ask for them (camera rays), never for shadows and bounces
bool candidateAccept(uint inst, uint prim, vec2 bary, bool includeSemi)
{
	TriData d = fetchTriangle(inst, prim, bary);
	if (d.texel.a < 0.5)
	{
		return false;
	}
	return includeSemi || (d.blend == 3u);
}

// closest hit; returns t < 0 on miss
float traceClosest(vec3 o, vec3 d, float tmax, bool includeSemi, out uint inst, out uint prim, out vec2 bary)
{
	rayQueryEXT rq;
	rayQueryInitializeEXT(rq, tlas, gl_RayFlagsNoneEXT, 0xff, o, 0.0, d, tmax);
	while (rayQueryProceedEXT(rq))
	{
		if (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
		{
			uint ci = uint(rayQueryGetIntersectionInstanceCustomIndexEXT(rq, false));
			uint cp = uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, false));
			if (candidateAccept(ci, cp, rayQueryGetIntersectionBarycentricsEXT(rq, false), includeSemi))
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
	return rayQueryGetIntersectionTEXT(rq, true);
}

bool occluded(vec3 o, vec3 d, float tmax)
{
	rayQueryEXT rq;
	rayQueryInitializeEXT(rq, tlas, gl_RayFlagsTerminateOnFirstHitEXT, 0xff, o, 0.0, d, tmax);
	while (rayQueryProceedEXT(rq))
	{
		if (rayQueryGetIntersectionTypeEXT(rq, false) == gl_RayQueryCandidateIntersectionTriangleEXT)
		{
			uint ci = uint(rayQueryGetIntersectionInstanceCustomIndexEXT(rq, false));
			uint cp = uint(rayQueryGetIntersectionPrimitiveIndexEXT(rq, false));
			if (candidateAccept(ci, cp, rayQueryGetIntersectionBarycentricsEXT(rq, false), false))
			{
				rayQueryConfirmIntersectionEXT(rq);
			}
		}
	}
	return rayQueryGetIntersectionTypeEXT(rq, true) == gl_RayQueryCommittedIntersectionTriangleEXT;
}

struct Surface
{
	vec3 pos;
	vec3 geoNormal; // facing the incoming ray: ray offsets
	vec3 normal;    // smooth shading normal, same side
	vec3 albedo;    // linear
	vec3 ps1Color;  // texel x vertex colour as the PS1 draws it (display space, 0..2)
	uint flags;
	uint blend;
};

vec3 srgbToLinear(vec3 c) { return pow(max(c, vec3(0.0)), vec3(2.2)); }
vec3 linearToSrgb(vec3 c) { return pow(max(c, vec3(0.0)), vec3(1.0 / 2.2)); }

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
	// PS1 modulation: texel * vertex colour / 0x80; level vertex colours carry the
	// artists' baked light, so only part of them is kept as surface tint
	float mixv = F.skyZenith.w;
	vec3 tint = clamp(mix(vec3(1.0), d.vcol, (inst == INST_LEVEL) ? mixv : 1.0), 0.0, 2.0);
	bool textured = (d.flags & TRI_TEXTURED) != 0u;
	vec3 base = textured ? d.texel.rgb * tint : clamp(d.vcol * 0.5, 0.0, 1.0);
	s.albedo = min(srgbToLinear(base), vec3(0.95));
	s.ps1Color = textured ? d.texel.rgb * d.vcol : clamp(d.vcol * 0.5, 0.0, 1.0);
	s.flags = d.flags;
	s.blend = d.blend;
	return s;
}

// ---- random numbers --------------------------------------------------------

uint pcg(inout uint state)
{
	state = state * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float rnd(inout uint state) { return float(pcg(state)) * (1.0 / 4294967296.0); }

vec3 cosineSample(vec3 n, inout uint rng)
{
	float u1 = rnd(rng), u2 = rnd(rng);
	float r = sqrt(u1), phi = 6.28318530718 * u2;
	vec3 t = normalize(abs(n.y) < 0.99 ? cross(n, vec3(0, 1, 0)) : cross(n, vec3(1, 0, 0)));
	vec3 b = cross(n, t);
	return normalize(t * (r * cos(phi)) + b * (r * sin(phi)) + n * sqrt(max(0.0, 1.0 - u1)));
}

vec3 coneSample(vec3 d, float angle, inout uint rng)
{
	float cosMax = cos(angle);
	float z = mix(cosMax, 1.0, rnd(rng));
	float r = sqrt(max(0.0, 1.0 - z * z));
	float phi = 6.28318530718 * rnd(rng);
	vec3 t = normalize(abs(d.y) < 0.99 ? cross(d, vec3(0, 1, 0)) : cross(d, vec3(1, 0, 0)));
	vec3 b = cross(d, t);
	return normalize(t * (r * cos(phi)) + b * (r * sin(phi)) + d * z);
}

vec3 skyRadiance(vec3 d)
{
	if (F.ambient.w > 0.5)
	{
		return F.ambient.rgb;
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

// one shadowed sample from lights[first, first+count): 8 candidates picked by
// their unshadowed contribution (resampled importance sampling), one shadow ray
vec3 sampleLightSet(uint first, uint count, vec3 p, vec3 n, vec3 ng, float eps, inout uint rng)
{
	if (count == 0u)
	{
		return vec3(0.0);
	}
	const int M = 8;
	float wsum = 0.0, chosenW = 0.0;
	uint chosen = first;
	for (int i = 0; i < M; i++)
	{
		uint j = first + pcg(rng) % count;
		vec4 l0 = lights[j * 3u], l1 = lights[j * 3u + 1u];
		vec3 toL = l0.xyz - p;
		float d2 = max(dot(toL, toL), 1.0);
		float ndl = max(dot(n, toL * inversesqrt(d2)), 0.0);
		float w = (d2 < 36.0 * l1.w * l1.w) ? luma(l1.rgb) * ndl / (1.0 + d2 / (l1.w * l1.w)) : 0.0;
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
	if ((ndl <= 0.0) || (dot(ng, dir) <= 0.0) || occluded(p + ng * eps, dir, dist - eps * 2.0))
	{
		return vec3(0.0);
	}
	vec3 f = l1.rgb * ndl / (1.0 + (dist * dist) / (l1.w * l1.w));
	// RIS estimate f/p_hat * mean(p_hat); the caller multiplies by count (uniform source pdf 1/count)
	return f / chosenW * (wsum / float(M));
}

// direct light at a surface: sun (outdoors), static lights, emitters seen this frame
vec3 lightSample(vec3 p, vec3 n, vec3 ng, inout uint rng)
{
	vec3 result = vec3(0.0);
	float eps = rayEps(p);

	if (F.ambient.w < 0.5)
	{
		vec3 l = coneSample(F.sunDir.xyz, F.sunDir.w, rng);
		float ndl = dot(n, l);
		if ((ndl > 0.0) && (dot(ng, l) > 0.0) && !occluded(p + ng * eps, l, 1e7))
		{
			result += F.sunColor.rgb * ndl;
		}
	}
	uint numStatic = F.counts.x, numDyn = F.extra.x;
	result += sampleLightSet(0u, numStatic, p, n, ng, eps, rng) * float(numStatic);
	result += sampleLightSet(numStatic, numDyn, p, n, ng, eps, rng) * float(numDyn);
	return result;
}
