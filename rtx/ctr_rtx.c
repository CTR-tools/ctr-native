// ctr_rtx: 64-bit Vulkan ray-query renderer for CTR MultiTrack native.
//
// Started by the 32-bit game (platform/native_rtx.c) with the shared memory
// name as argv[1]. Builds a BLAS for the static level once per level, a BLAS
// for the per-frame captured model triangles, traces with VK_KHR_ray_query
// compute shaders (rtx/shaders) and hands back an RGBA8 image.
//
// Build: scripts/build_native.sh (gcc -O2, volk + Vulkan-Headers, glslc).

#define VOLK_IMPLEMENTATION
#include "volk.h"

#include "rtx_shared.h"

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/futex.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

#define CHECK(x)                                                                              \
	do                                                                                        \
	{                                                                                         \
		VkResult r_ = (x);                                                                    \
		if (r_ != VK_SUCCESS)                                                                 \
		{                                                                                     \
			fprintf(stderr, "[ctr_rtx] %s failed: %d (%s:%d)\n", #x, r_, __FILE__, __LINE__); \
			exit(1);                                                                          \
		}                                                                                     \
	} while (0)

static void logf_(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "[ctr_rtx] ");
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
	va_end(ap);
}

typedef struct
{
	VkBuffer buf;
	VkDeviceMemory mem;
	VkDeviceSize size;
	void *map;
	VkDeviceAddress addr;
} Buf;

typedef struct
{
	Buf buf;
	VkAccelerationStructureKHR as;
	VkDeviceAddress addr;
} Accel;

// mirrors FrameBuf in shaders/common.glsl (std430, vec4 aligned)
typedef struct
{
	float rInv[3][4];
	float camPos[4];
	float camR[3][4];
	float camT[4];
	float prevR[3][4];
	float prevT[4];
	float proj[4];
	float proj2[4];
	uint32_t size[4];
	float sunDir[4];
	float sunColor[4];
	float skyZenith[4];
	float skyHorizon[4];
	float groundColor[4];
	float ambient[4];
	uint32_t counts[4];
	float params[4];
	uint32_t extra[4];
	float params2[4];
	uint32_t extra2[4];
	float gridMin[4];
	uint32_t gridDim[4];
	float sky[64][4];
	uint32_t outSize[4]; // output w, h, TAA on
	float jitter[4];     // camera ray offset within its pixel (TAA)
	uint32_t feat[4];    // texture filter, specular, fog
} FrameData;

enum
{
	B_TLAS,
	B_FRAME,
	B_LEVEL_VERTS,
	B_LEVEL_TRIS,
	B_TEX_SLOTS,
	B_DYN_TRIS,
	B_VRAM,
	B_LIGHTS,
	B_ALBEDO,
	B_GEO_CUR,
	B_GEO_PREV,
	B_HIST_PREV,
	B_HIST_CUR,
	B_SRC,
	B_DST,
	B_OUT,
	B_LUM,
	B_LEVEL_NORMALS,
	B_DYN_NORMALS,
	B_LAYER,
	B_DEPTH,
	B_CACHE_KEYS,
	B_CACHE_ACCUM,
	B_CACHE_VAL,
	B_LIGHT_GRID,
	B_FIT,
	B_CAP_TRIS,
	B_HIST_DYN_PREV,
	B_HIST_DYN_CUR,
	B_COMBINED,
	B_BAKE,
	B_LEVEL_TINT,
	B_MOM_PREV,
	B_MOM_CUR,
	B_RES_PREV,
	B_RES_CUR,
	B_DISP,     // display colour at the traced size (final, bloom -> taa)
	B_TAA_PREV, // output size: last frame's resolved image
	B_TAA_CUR,
	B_BLUE_NOISE, // 64x64 void-and-cluster ranks (sampling)
	B_SPEC_PREV,  // specular light, its own short history
	B_SPEC_CUR,
	B_FOG_PREV,   // air's light (haze, sun shafts), accumulated
	B_FOG_CUR,
	B_COUNT
};

enum
{
	PASS_TRACE,
	PASS_ATROUS1, // .. PASS_ATROUS1 + NUM_ATROUS - 1
	PASS_FINAL = PASS_ATROUS1 + 5,
	PASS_BLOOM_H,
	PASS_BLOOM_V,
	PASS_TAA,
	PASS_COUNT
};
#define NUM_ATROUS 5

#define CACHE_ENTRIES    (1u << 20) // radiance cache cells
#define LIGHT_GRID_WORDS (1u << 21) // static light grid: cell (offset, count) pairs + light indices
#define FIT_SAMPLES      4096u      // sun fit: sample vertices
#define FIT_DIRS         1024u      // and candidate directions
#define FIT_VEC4S        (1u + FIT_SAMPLES * 3u + FIT_DIRS * 3u)

static struct
{
	VkInstance instance;
	VkPhysicalDevice phys;
	VkDevice dev;
	VkQueue queue;
	uint32_t queueFamily;
	VkPhysicalDeviceMemoryProperties memProps;
	VkPhysicalDeviceAccelerationStructurePropertiesKHR asProps;
	VkCommandPool pool;
	VkCommandBuffer cmd;
	VkFence fence;
	VkQueryPool timestamps; // between the passes (TS_*): the trace pass's drives samples per pixel
	float timestampPeriodNs;
	int haveTimestamps;

	VkDescriptorSetLayout setLayout;
	VkPipelineLayout pipeLayout;
	VkDescriptorPool descPool;
	VkDescriptorSet sets[2][PASS_COUNT];
	VkPipeline pipeTrace, pipeAtrous, pipeFinal, pipeLights, pipeCache, pipeSunfit, pipeBloom, pipeBake, pipeTaa;

	// shared memory
	uint8_t *shm;
	struct RtxHeader *hdr;

	// level
	uint32_t levelGen;
	int haveLevel;
	Buf levelVerts, levelTris, levelIdx, lights, lightsStage, lightsReadback, levelNormals;
	Accel levelBlas;
	int levelPrep;        // level loaded, its lights not worked out yet (needs descriptor sets)
	float *levelNrm;      // unit vertex normals (sign arbitrary), 4 floats per vertex
	uint32_t cornerBase;  // level normals buffer: per vertex (bake), then per triangle corner from here (shading)
	float levelFront;     // 1: the corner normals are oriented (back-face culling for under-terrain shadow rays may use them)
	uint32_t numLights;   // static: lights file, glowing surfaces, the art's lamps (hotspots)
	uint32_t numFileLights, numGlowLights;
	float gridMin[4];     // static light grid
	uint32_t gridDim[3];
	float emissiveGain;
	int sunFit, hotspots, ambientSet;
	uint32_t skyValid;
	float sky[64][4]; // linear, scaled to the fitted sky light

	// radiance cache (hash grid) and the static light grid
	Buf cacheKeys, cacheAccum, cacheVal, lightGrid, fit, bake, levelTint, levelTintStage;
	float tintLum, tintChroma; // how much of the art's brightness / colour stays in the surface
	float sunMin;              // outdoors: sun at least this much of the sky light

	// per frame
	Buf frame, texSlots, texSlotsStage, dynTris, dynTrisStage, dynPos, dynPosStage, dynNormals, dynNormalsStage, vram, vramStage, tlasInstances, scratch, lum;
	Buf capTris, capTrisStage, capPos, capPosStage; // the level as the PS1 drew it last frame (texture layer)
	Accel dynBlas, capBlas, tlas;

	// per pixel: traced at inW x inH (RtxSettings.renderScale), resolved by TAA onto outW x outH
	uint32_t outW, outH, inW, inH;
	Buf disp, taaHist[2], blueNoise, spec[2], fog[2];
	Buf albedo, geo[2], hist[2], histDyn[2], moments[2], reservoir[2], combined, filterA, filterB, layer, out, readback, depth, depthReadback;
	uint32_t parity;
	uint32_t frameIndex;

	// lighting
	struct RtxLighting light;
	float prevR[9], prevT[3], prevH, prevCamPos[3];
	int havePrev;
	uint32_t lastGameFrame;
	uint32_t lastLightMode;
	float autoExposure;

	// samples per pixel: the quality preset's, lowered when the trace pass would
	// take longer than its budget (RtxSettings)
	uint32_t spp;
	float traceMsPerSpp;
	double statTraceMs, statSppMs; // debug: trace pass sums since the last report
	uint32_t statFrames;
	float budgetMs;
	double statPass[RTX_PASS_COUNT]; // debug: pass time sums since the last report
	double cpuMs;                    // this frame's renderer CPU time so far
} R;

// timestamps written between the passes of a frame
enum
{
	TS_START,
	TS_BUILT,  // uploads + acceleration structures
	TS_TRACED,
	TS_CACHED,
	TS_FILTERED,
	TS_FINISHED, // composite, TAA, bloom
	TS_COPIED,
	TS_COUNT
};

static void stamp(int i)
{
	if (R.haveTimestamps)
	{
		vkCmdWriteTimestamp(R.cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, R.timestamps, (uint32_t)i);
	}
}

// ---- small helpers -----------------------------------------------------------

// sleep while *addr == val (at most ms), wake the other side's sleeper (rtx_shared.h)
static void futexWait(volatile uint32_t *addr, uint32_t val, long ms)
{
	struct timespec ts = {ms / 1000, (ms % 1000) * 1000000L};
	syscall(SYS_futex, (uint32_t *)addr, FUTEX_WAIT, val, &ts, NULL, 0);
}

static void futexWake(volatile uint32_t *addr)
{
	syscall(SYS_futex, (uint32_t *)addr, FUTEX_WAKE, 0x7fffffff, NULL, NULL, 0);
}

static double nowMs(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

static uint32_t memType(uint32_t bits, VkMemoryPropertyFlags want)
{
	for (uint32_t i = 0; i < R.memProps.memoryTypeCount; i++)
	{
		if ((bits & (1u << i)) && ((R.memProps.memoryTypes[i].propertyFlags & want) == want))
		{
			return i;
		}
	}
	logf_("no memory type for flags %x", want);
	exit(1);
}

static Buf bufCreate(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props)
{
	Buf b = {0};
	if (size < 256)
	{
		size = 256;
	}
	b.size = size;
	usage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	VkBufferCreateInfo bi = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = size, .usage = usage, .sharingMode = VK_SHARING_MODE_EXCLUSIVE};
	CHECK(vkCreateBuffer(R.dev, &bi, NULL, &b.buf));
	VkMemoryRequirements req;
	vkGetBufferMemoryRequirements(R.dev, b.buf, &req);
	VkMemoryAllocateFlagsInfo flags = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO, .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT};
	VkMemoryAllocateInfo ai = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, &flags, req.size, memType(req.memoryTypeBits, props)};
	CHECK(vkAllocateMemory(R.dev, &ai, NULL, &b.mem));
	CHECK(vkBindBufferMemory(R.dev, b.buf, b.mem, 0));
	if (props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	{
		CHECK(vkMapMemory(R.dev, b.mem, 0, VK_WHOLE_SIZE, 0, &b.map));
	}
	VkBufferDeviceAddressInfo dai = {VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = b.buf};
	b.addr = vkGetBufferDeviceAddress(R.dev, &dai);
	return b;
}

static void bufDestroy(Buf *b)
{
	if (b->buf)
	{
		vkDestroyBuffer(R.dev, b->buf, NULL);
		vkFreeMemory(R.dev, b->mem, NULL);
	}
	memset(b, 0, sizeof(*b));
}

#define USAGE_STORAGE (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
#define USAGE_AS_INPUT (VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR)
#define DEVICE_LOCAL VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
#define HOST_MEM (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)

static void cmdBegin(void)
{
	CHECK(vkResetCommandBuffer(R.cmd, 0));
	VkCommandBufferBeginInfo bi = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO, .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
	CHECK(vkBeginCommandBuffer(R.cmd, &bi));
}

static void cmdSubmitWait(void)
{
	CHECK(vkEndCommandBuffer(R.cmd));
	VkSubmitInfo si = {VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &R.cmd};
	CHECK(vkResetFences(R.dev, 1, &R.fence));
	CHECK(vkQueueSubmit(R.queue, 1, &si, R.fence));
	CHECK(vkWaitForFences(R.dev, 1, &R.fence, VK_TRUE, UINT64_MAX));
}

static void barrier(VkPipelineStageFlags src, VkAccessFlags srcAccess, VkPipelineStageFlags dst, VkAccessFlags dstAccess)
{
	VkMemoryBarrier mb = {VK_STRUCTURE_TYPE_MEMORY_BARRIER, .srcAccessMask = srcAccess, .dstAccessMask = dstAccess};
	vkCmdPipelineBarrier(R.cmd, src, dst, 0, 1, &mb, 0, NULL, 0, NULL);
}

static void copyBuf(Buf *src, Buf *dst, VkDeviceSize size)
{
	if (size == 0)
	{
		return;
	}
	VkBufferCopy c = {0, 0, size};
	vkCmdCopyBuffer(R.cmd, src->buf, dst->buf, 1, &c);
}

// one-shot upload of host data into a new device-local buffer
static Buf uploadBuf(const void *data, VkDeviceSize size, VkBufferUsageFlags usage)
{
	Buf dst = bufCreate(size, usage | USAGE_STORAGE, DEVICE_LOCAL);
	Buf stage = bufCreate(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	if (size)
	{
		memcpy(stage.map, data, size);
	}
	cmdBegin();
	copyBuf(&stage, &dst, size);
	cmdSubmitWait();
	bufDestroy(&stage);
	return dst;
}

// ---- Vulkan setup ------------------------------------------------------------

static void initVulkan(void)
{
	CHECK(volkInitialize());
	VkApplicationInfo app = {VK_STRUCTURE_TYPE_APPLICATION_INFO, .pApplicationName = "ctr_rtx", .apiVersion = VK_API_VERSION_1_3};
	VkInstanceCreateInfo ici = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app};
	CHECK(vkCreateInstance(&ici, NULL, &R.instance));
	volkLoadInstance(R.instance);

	uint32_t n = 8;
	VkPhysicalDevice devs[8];
	CHECK(vkEnumeratePhysicalDevices(R.instance, &n, devs));
	const char *exts[] = {VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, VK_KHR_RAY_QUERY_EXTENSION_NAME, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};
	// discrete GPU first (the AMD iGPU also has ray queries under RADV)
	for (uint32_t pass = 0; pass < 2 && !R.phys; pass++)
	for (uint32_t i = 0; i < n && !R.phys; i++)
	{
		VkPhysicalDeviceProperties dp;
		vkGetPhysicalDeviceProperties(devs[i], &dp);
		if ((pass == 0) && (dp.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU))
		{
			continue;
		}
		uint32_t ne = 0;
		vkEnumerateDeviceExtensionProperties(devs[i], NULL, &ne, NULL);
		VkExtensionProperties *ep = calloc(ne, sizeof(*ep));
		vkEnumerateDeviceExtensionProperties(devs[i], NULL, &ne, ep);
		int found = 0;
		for (int e = 0; e < 3; e++)
		{
			for (uint32_t k = 0; k < ne; k++)
			{
				if (!strcmp(ep[k].extensionName, exts[e]))
				{
					found++;
					break;
				}
			}
		}
		free(ep);
		if (found == 3)
		{
			R.phys = devs[i];
		}
	}
	if (!R.phys)
	{
		logf_("no GPU with VK_KHR_ray_query found");
		exit(2);
	}
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(R.phys, &props);
	vkGetPhysicalDeviceMemoryProperties(R.phys, &R.memProps);
	R.asProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR;
	VkPhysicalDeviceProperties2 p2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, &R.asProps};
	vkGetPhysicalDeviceProperties2(R.phys, &p2);
	logf_("GPU: %s", props.deviceName);

	uint32_t nq = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(R.phys, &nq, NULL);
	VkQueueFamilyProperties qp[16];
	if (nq > 16)
	{
		nq = 16;
	}
	vkGetPhysicalDeviceQueueFamilyProperties(R.phys, &nq, qp);
	R.queueFamily = UINT32_MAX;
	for (uint32_t i = 0; i < nq; i++)
	{
		if ((qp[i].queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)) == (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT))
		{
			R.queueFamily = i;
			break;
		}
	}
	float prio = 1.0f;
	VkDeviceQueueCreateInfo qci = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, .queueFamilyIndex = R.queueFamily, .queueCount = 1, .pQueuePriorities = &prio};
	VkPhysicalDeviceRayQueryFeaturesKHR rq = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR, .rayQuery = VK_TRUE};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR asf = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR, &rq, .accelerationStructure = VK_TRUE};
	VkPhysicalDeviceVulkan12Features v12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &asf, .bufferDeviceAddress = VK_TRUE};
	VkDeviceCreateInfo dci = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, &v12, .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci, .enabledExtensionCount = 3,
	                          .ppEnabledExtensionNames = exts};
	CHECK(vkCreateDevice(R.phys, &dci, NULL, &R.dev));
	volkLoadDevice(R.dev);
	vkGetDeviceQueue(R.dev, R.queueFamily, 0, &R.queue);

	VkCommandPoolCreateInfo pci = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT, .queueFamilyIndex = R.queueFamily};
	CHECK(vkCreateCommandPool(R.dev, &pci, NULL, &R.pool));
	VkCommandBufferAllocateInfo cai = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, .commandPool = R.pool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1};
	CHECK(vkAllocateCommandBuffers(R.dev, &cai, &R.cmd));
	VkFenceCreateInfo fci = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
	CHECK(vkCreateFence(R.dev, &fci, NULL, &R.fence));

	R.haveTimestamps = (qp[R.queueFamily].timestampValidBits > 0) && (props.limits.timestampPeriod > 0.0f);
	R.timestampPeriodNs = props.limits.timestampPeriod;
	if (R.haveTimestamps)
	{
		VkQueryPoolCreateInfo qci2 = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, .queryType = VK_QUERY_TYPE_TIMESTAMP, .queryCount = TS_COUNT};
		CHECK(vkCreateQueryPool(R.dev, &qci2, NULL, &R.timestamps));
	}
	R.budgetMs = 28.0f; // until the game's settings arrive (RtxSettings.budgetMs)
	R.spp = 2;
}

static char s_exeDir[1024];

static VkShaderModule loadShader(const char *name)
{
	char path[1200];
	snprintf(path, sizeof(path), "%s/rtx_shaders/%s.spv", s_exeDir, name);
	FILE *f = fopen(path, "rb");
	if (!f)
	{
		logf_("missing shader %s", path);
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint32_t *code = malloc(size);
	if (fread(code, 1, size, f) != (size_t)size)
	{
		logf_("short read %s", path);
		exit(1);
	}
	fclose(f);
	VkShaderModuleCreateInfo ci = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, .codeSize = (size_t)size, .pCode = code};
	VkShaderModule m;
	CHECK(vkCreateShaderModule(R.dev, &ci, NULL, &m));
	free(code);
	return m;
}

static VkPipeline makePipeline(const char *name)
{
	VkShaderModule m = loadShader(name);
	VkComputePipelineCreateInfo ci = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
	                                  .stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO, .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = m, .pName = "main"},
	                                  .layout = R.pipeLayout};
	VkPipeline p;
	CHECK(vkCreateComputePipelines(R.dev, VK_NULL_HANDLE, 1, &ci, NULL, &p));
	vkDestroyShaderModule(R.dev, m, NULL);
	return p;
}

static void initPipelines(void)
{
	VkDescriptorSetLayoutBinding b[B_COUNT];
	for (int i = 0; i < B_COUNT; i++)
	{
		b[i] = (VkDescriptorSetLayoutBinding){(uint32_t)i, i == B_TLAS ? VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
		                                      VK_SHADER_STAGE_COMPUTE_BIT, NULL};
	}
	VkDescriptorSetLayoutCreateInfo lci = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = B_COUNT, .pBindings = b};
	CHECK(vkCreateDescriptorSetLayout(R.dev, &lci, NULL, &R.setLayout));
	VkPushConstantRange pcr = {VK_SHADER_STAGE_COMPUTE_BIT, 0, 4};
	VkPipelineLayoutCreateInfo pli = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO, .setLayoutCount = 1, .pSetLayouts = &R.setLayout, .pushConstantRangeCount = 1,
	                                  .pPushConstantRanges = &pcr};
	CHECK(vkCreatePipelineLayout(R.dev, &pli, NULL, &R.pipeLayout));

	VkDescriptorPoolSize ps[2] = {{VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 2 * PASS_COUNT}, {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 * PASS_COUNT * B_COUNT}};
	VkDescriptorPoolCreateInfo dpi = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, .maxSets = 2 * PASS_COUNT, .poolSizeCount = 2, .pPoolSizes = ps};
	CHECK(vkCreateDescriptorPool(R.dev, &dpi, NULL, &R.descPool));
	VkDescriptorSetLayout layouts[2 * PASS_COUNT];
	for (int i = 0; i < 2 * PASS_COUNT; i++)
	{
		layouts[i] = R.setLayout;
	}
	VkDescriptorSetAllocateInfo dai = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO, .descriptorPool = R.descPool, .descriptorSetCount = 2 * PASS_COUNT,
	                                   .pSetLayouts = layouts};
	CHECK(vkAllocateDescriptorSets(R.dev, &dai, &R.sets[0][0]));

	R.pipeTrace = makePipeline("trace");
	R.pipeAtrous = makePipeline("atrous");
	R.pipeFinal = makePipeline("final");
	R.pipeLights = makePipeline("lights");
	R.pipeCache = makePipeline("cache");
	R.pipeSunfit = makePipeline("sunfit");
	R.pipeBloom = makePipeline("bloom");
	R.pipeBake = makePipeline("bake");
	R.pipeTaa = makePipeline("taa");
}

// ---- acceleration structures -----------------------------------------------------

static void accelDestroy(Accel *a)
{
	if (a->as)
	{
		vkDestroyAccelerationStructureKHR(R.dev, a->as, NULL);
	}
	bufDestroy(&a->buf);
	memset(a, 0, sizeof(*a));
}

static void accelCreate(Accel *a, VkAccelerationStructureTypeKHR type, VkDeviceSize size)
{
	a->buf = bufCreate(size, VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR, DEVICE_LOCAL);
	VkAccelerationStructureCreateInfoKHR ci = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR, .buffer = a->buf.buf, .size = size, .type = type};
	CHECK(vkCreateAccelerationStructureKHR(R.dev, &ci, NULL, &a->as));
	VkAccelerationStructureDeviceAddressInfoKHR ai = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, .accelerationStructure = a->as};
	a->addr = vkGetAccelerationStructureDeviceAddressKHR(R.dev, &ai);
}

static VkDeviceAddress scratchAddr(VkDeviceSize need)
{
	VkDeviceSize align = R.asProps.minAccelerationStructureScratchOffsetAlignment;
	if (R.scratch.size < need + align)
	{
		vkDeviceWaitIdle(R.dev);
		bufDestroy(&R.scratch);
		R.scratch = bufCreate(need + align, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, DEVICE_LOCAL);
	}
	return (R.scratch.addr + align - 1) & ~(align - 1);
}

static VkAccelerationStructureGeometryKHR triGeometry(VkDeviceAddress verts, VkDeviceSize stride, uint32_t maxVertex, VkDeviceAddress indices)
{
	VkAccelerationStructureGeometryKHR g = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR};
	g.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	g.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	g.geometry.triangles.vertexData.deviceAddress = verts;
	g.geometry.triangles.vertexStride = stride;
	g.geometry.triangles.maxVertex = maxVertex;
	g.geometry.triangles.indexType = indices ? VK_INDEX_TYPE_UINT32 : VK_INDEX_TYPE_NONE_KHR;
	g.geometry.triangles.indexData.deviceAddress = indices;
	g.flags = 0; // not opaque: candidates get the PS1 colour-key alpha test
	return g;
}

// records a BLAS build (creating/growing the AS if needed); needs an open command buffer
static void buildBlas(Accel *a, VkAccelerationStructureGeometryKHR *geom, uint32_t primCount, uint32_t maxPrims, VkBuildAccelerationStructureFlagsKHR flags)
{
	VkAccelerationStructureBuildGeometryInfoKHR bi = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	                                                   .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	                                                   .flags = flags,
	                                                   .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	                                                   .geometryCount = 1,
	                                                   .pGeometries = geom};
	VkAccelerationStructureBuildSizesInfoKHR sizes = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	vkGetAccelerationStructureBuildSizesKHR(R.dev, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &bi, &maxPrims, &sizes);
	if (!a->as || a->buf.size < sizes.accelerationStructureSize)
	{
		accelDestroy(a);
		accelCreate(a, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, sizes.accelerationStructureSize);
	}
	bi.dstAccelerationStructure = a->as;
	bi.scratchData.deviceAddress = scratchAddr(sizes.buildScratchSize);
	VkAccelerationStructureBuildRangeInfoKHR range = {.primitiveCount = primCount};
	const VkAccelerationStructureBuildRangeInfoKHR *pr = &range;
	vkCmdBuildAccelerationStructuresKHR(R.cmd, 1, &bi, &pr);
}

// ---- lights file -------------------------------------------------------------------

static void defaultLighting(struct RtxLighting *L)
{
	memset(L, 0, sizeof(*L));
	float el = 40.0f * (float)M_PI / 180.0f, az = 35.0f * (float)M_PI / 180.0f;
	L->sunDir[0] = cosf(el) * cosf(az);
	L->sunDir[1] = sinf(el);
	L->sunDir[2] = cosf(el) * sinf(az);
	L->sunCone = 0.3f * (float)M_PI / 180.0f; // the sun's disc: penumbras grow with the occluder's distance
	float sun[3] = {1.0f, 0.93f, 0.80f};
	for (int i = 0; i < 3; i++)
	{
		L->sunColor[i] = sun[i] * 2.6f;
	}
	float zen[3] = {0.30f, 0.38f, 0.55f}, hor[3] = {0.45f, 0.48f, 0.55f}, gnd[3] = {0.10f, 0.09f, 0.08f}, amb[3] = {0.40f, 0.42f, 0.48f};
	memcpy(L->skyZenith, zen, sizeof(zen));
	memcpy(L->skyHorizon, hor, sizeof(hor));
	memcpy(L->ground, gnd, sizeof(gnd));
	memcpy(L->ambient, amb, sizeof(amb));
	L->exposure = 0.0f; // auto from the sun/sky
}

// assets/rtx_lights/<key>.txt, written by scripts/rt_bake.py --export-lights.
// The lighting comes from the level's art (levelPrep); the file says whether
// the track is indoors and can override:
//   indoor 0|1
//   sun dx dy dz cone r g b     (with "sunfit 0": else fitted to the vertex colours)
//   sky zr zg zb hr hg hb
//   ground r g b
//   ambient r g b               (indoor; else from the vertex colours)
//   exposure e                  (else matched to the PS1 frame)
//   emissive_gain g             (glowing surfaces, default 24)
//   sun_min k                   (outdoors: sun at least k x the sky light, default 2)
//   tint_lum k, tint_chroma k   (how much of the art's brightness beyond the fitted
//                                light / local colour stays in the surface; 0.25, 1)
//   sunfit 0|1, hotspots 0|1
//   light x y z height r g b falloff size
// one lighting file's lines into L (and the R.* settings)
static void loadLightingFile(const char *path, struct RtxLighting *L)
{
	FILE *f = fopen(path, "r");
	if (!f)
	{
		return;
	}
	char line[512];
	while (fgets(line, sizeof(line), f))
	{
		int v;
		struct RtxPointLight pl;
		if (sscanf(line, "indoor %d", &v) == 1)
		{
			L->indoor = (uint32_t)v;
		}
		else if (sscanf(line, "sun %f %f %f %f %f %f %f", &L->sunDir[0], &L->sunDir[1], &L->sunDir[2], &L->sunCone, &L->sunColor[0], &L->sunColor[1],
		                &L->sunColor[2]) == 7)
		{
		}
		else if (sscanf(line, "sky %f %f %f %f %f %f", &L->skyZenith[0], &L->skyZenith[1], &L->skyZenith[2], &L->skyHorizon[0], &L->skyHorizon[1],
		                &L->skyHorizon[2]) == 6)
		{
		}
		else if (sscanf(line, "ground %f %f %f", &L->ground[0], &L->ground[1], &L->ground[2]) == 3)
		{
		}
		else if (sscanf(line, "ambient %f %f %f", &L->ambient[0], &L->ambient[1], &L->ambient[2]) == 3)
		{
			R.ambientSet = 1;
		}
		else if (sscanf(line, "emissive_gain %f", &R.emissiveGain) == 1)
		{
		}
		else if (sscanf(line, "tint_lum %f", &R.tintLum) == 1)
		{
		}
		else if (sscanf(line, "tint_chroma %f", &R.tintChroma) == 1)
		{
		}
		else if (sscanf(line, "sun_min %f", &R.sunMin) == 1)
		{
		}
		else if (sscanf(line, "sunfit %d", &R.sunFit) == 1)
		{
		}
		else if (sscanf(line, "hotspots %d", &R.hotspots) == 1)
		{
		}
		else if (sscanf(line, "exposure %f", &L->exposure) == 1)
		{
		}
		else if ((sscanf(line, "light %f %f %f %f %f %f %f %f %f", &pl.pos[0], &pl.pos[1], &pl.pos[2], &pl.height, &pl.color[0], &pl.color[1], &pl.color[2],
		                 &pl.falloff, &pl.size) == 9) &&
		         (L->numLights < RTX_MAX_LIGHTS))
		{
			L->lights[L->numLights++] = pl;
		}
	}
	fclose(f);
}

static void loadLighting(const char *key)
{
	defaultLighting(&R.light);
	R.emissiveGain = 24.0f;
	R.tintLum = 0.25f;
	R.tintChroma = 1.0f;
	R.sunMin = 2.0f;
	R.sunFit = 1;
	R.hotspots = 1;
	R.ambientSet = 0;
	struct RtxLighting *L = &R.light;
	char path[1400];
	snprintf(path, sizeof(path), "%s/assets/rtx_lights/%s.txt", s_exeDir, key);
	FILE *f = fopen(path, "r");
	if (!f)
	{
		logf_("no %s, default outdoor lighting", path);
		return;
	}
	fclose(f);
	loadLightingFile(path, L);
	// per-level overrides (native/rtx/overrides/<key>.override.txt), read after so they win
	snprintf(path, sizeof(path), "%s/assets/rtx_lights/%s.override.txt", s_exeDir, key);
	if ((f = fopen(path, "r")) != NULL)
	{
		fclose(f);
		loadLightingFile(path, L);
		logf_("overrides from %s", path);
	}
	logf_("lighting %s: %s, %u lights", key, L->indoor ? "indoor" : "outdoor", L->numLights);
}

static float luma3(const float *c)
{
	return 0.299f * c[0] + 0.587f * c[1] + 0.114f * c[2];
}

// ---- smooth normals -------------------------------------------------------------------

static void cross3(const float *a, const float *b, float *out)
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

static void faceNormal(const float *p0, const float *p1, const float *p2, float *n)
{
	float e1[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]}, e2[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
	cross3(e1, e2, n);
}

// Smoke particles come as camera-facing quads (two triangles: corners 0 1 2 and
// 1 3 2), only there for shadows. Seen from the sun they would be edge-on unless
// the sun is behind the camera: turn each to face the sun, same centre and size
static void smokeFaceSun(struct RtxDynTri *tris, uint32_t n, const float *sun)
{
	float up[3] = {0.0f, 1.0f, 0.0f};
	if (fabsf(sun[1]) > 0.99f)
	{
		up[0] = 1.0f;
		up[1] = 0.0f;
	}
	float u[3], v[3];
	cross3(sun, up, u);
	float ul = sqrtf(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
	for (int k = 0; k < 3; k++)
	{
		u[k] /= ul;
	}
	cross3(u, sun, v);
	for (uint32_t i = 0; i + 1 < n; i++)
	{
		struct RtxDynTri *a = &tris[i], *b = &tris[i + 1];
		if (!(a->flags & RTX_TRI_SMOKE) || !(b->flags & RTX_TRI_SMOKE))
		{
			continue;
		}
		const float *q0 = a->p[0], *q1 = a->p[1], *q2 = a->p[2], *q3 = b->p[1];
		float c[3], hw = 0.0f, hh = 0.0f;
		for (int k = 0; k < 3; k++)
		{
			c[k] = 0.25f * (q0[k] + q1[k] + q2[k] + q3[k]);
			hw += (q1[k] - q0[k]) * (q1[k] - q0[k]);
			hh += (q2[k] - q0[k]) * (q2[k] - q0[k]);
		}
		hw = 0.5f * sqrtf(hw);
		hh = 0.5f * sqrtf(hh);
		float p[4][3];
		for (int k = 0; k < 3; k++)
		{
			p[0][k] = c[k] - u[k] * hw - v[k] * hh;
			p[1][k] = c[k] + u[k] * hw - v[k] * hh;
			p[2][k] = c[k] - u[k] * hw + v[k] * hh;
			p[3][k] = c[k] + u[k] * hw + v[k] * hh;
		}
		memcpy(a->p[0], p[0], 12);
		memcpy(a->p[1], p[1], 12);
		memcpy(a->p[2], p[2], 12);
		memcpy(b->p[0], p[1], 12);
		memcpy(b->p[1], p[3], 12);
		memcpy(b->p[2], p[2], 12);
		i++;
	}
}

// models: weld corners at the same position within one object. The PS1 only
// emits camera-facing polygons (back faces are culled), so every face normal is
// turned towards the camera before it is summed: consistent across the model
static void dynSmoothNormals(const struct RtxDynTri *tris, uint32_t n, float *out, const float *camPos)
{
	enum
	{
		HASH = 1 << 14
	};
	static int32_t keyX[HASH], keyY[HASH], keyZ[HASH];
	static uint32_t gen[HASH], curGen;
	static float acc[HASH][3];
	static uint32_t slot[RTX_MAX_DYN_TRIS * 3];

	uint32_t start = 0;
	while (start < n)
	{
		uint32_t end = start;
		while ((end < n) && (tris[end].object == tris[start].object) && (end - start < HASH / 4))
		{
			end++;
		}
		curGen++;
		for (uint32_t t = start; t < end; t++)
		{
			float fn[3];
			faceNormal(tris[t].p[0], tris[t].p[1], tris[t].p[2], fn);
			float toCam = (camPos[0] - tris[t].p[0][0]) * fn[0] + (camPos[1] - tris[t].p[0][1]) * fn[1] + (camPos[2] - tris[t].p[0][2]) * fn[2];
			if (toCam < 0.0f)
			{
				fn[0] = -fn[0];
				fn[1] = -fn[1];
				fn[2] = -fn[2];
			}
			for (int c = 0; c < 3; c++)
			{
				int32_t x = (int32_t)lrintf(tris[t].p[c][0]), y = (int32_t)lrintf(tris[t].p[c][1]), z = (int32_t)lrintf(tris[t].p[c][2]);
				uint32_t h = ((uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)z * 83492791u) & (HASH - 1);
				while ((gen[h] == curGen) && !((keyX[h] == x) && (keyY[h] == y) && (keyZ[h] == z)))
				{
					h = (h + 1) & (HASH - 1);
				}
				if (gen[h] != curGen)
				{
					gen[h] = curGen;
					keyX[h] = x;
					keyY[h] = y;
					keyZ[h] = z;
					acc[h][0] = acc[h][1] = acc[h][2] = 0.0f;
				}
				acc[h][0] += fn[0];
				acc[h][1] += fn[1];
				acc[h][2] += fn[2];
				slot[t * 3 + c] = h;
			}
		}
		for (uint32_t t = start; t < end; t++)
		{
			for (int c = 0; c < 3; c++)
			{
				const float *a = acc[slot[t * 3 + c]];
				float *o = &out[(t * 3 + c) * 4];
				o[0] = a[0];
				o[1] = a[1];
				o[2] = a[2];
				o[3] = 0.0f;
			}
		}
		start = end;
	}
}

// ---- glowing level surfaces -> static lights --------------------------------------------

static uint16_t shmVram16(int x, int y)
{
	const uint16_t *v = (const uint16_t *)(R.shm + RTX_OFF_VRAM);
	return v[(y & 511) * 1024 + (x & 1023)];
}

static int cpuTexel(uint16_t tpage, uint16_t clut, int u, int v, float *rgb)
{
	int mode = (tpage >> 7) & 3, bx = (tpage & 15) * 64, by = ((tpage >> 4) & 1) * 256, cx = (clut & 63) * 16, cy = (clut >> 6) & 511;
	u &= 255;
	v &= 255;
	uint16_t c;
	if (mode == 0)
	{
		c = shmVram16(cx + ((shmVram16(bx + (u >> 2), by + v) >> ((u & 3) * 4)) & 15), cy);
	}
	else if (mode == 1)
	{
		c = shmVram16(cx + ((shmVram16(bx + (u >> 1), by + v) >> ((u & 1) * 8)) & 255), cy);
	}
	else
	{
		c = shmVram16(bx + u, by + v);
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

// ---- lights from the art ------------------------------------------------------------
// The level's vertex colours are the artists' lighting (baked sun, shade, lamp
// pools, coloured glows) and its additive textures are what glows. From them:
//   - sun direction, colour and sky (outdoors): sunfit.comp
//   - ambient colour (indoors): the darker half of the vertex colours
//   - glowing surfaces: point lights per cluster, from their emitted power
//   - lamp pools ("hotspots"): vertices brighter than the ambient/sun explains,
//     clustered; a light is placed off each pool and its colour fitted so the
//     traced light reproduces the pool
// All in light units: linear vertex colour (0x80 = 1), what the PS1 multiplies
// the texture by.

static float srgbLin(float c)
{
	return powf(fmaxf(c, 0.0f), 2.2f);
}

static void vertLinColor(const struct RtxLevelVert *v, float *c)
{
	c[0] = srgbLin((v->rgba & 255) / 128.0f);
	c[1] = srgbLin(((v->rgba >> 8) & 255) / 128.0f);
	c[2] = srgbLin(((v->rgba >> 16) & 255) / 128.0f);
}

static int cmpFloat(const void *a, const void *b)
{
	float x = *(const float *)a, y = *(const float *)b;
	return (x > y) - (x < y);
}

// one static light record for the lights buffer (and what lights.comp lifts)
static void putLight(float *d, const float *pos, float height, const float *color, float falloff, float size, const float *dir)
{
	d[0] = pos[0];
	d[1] = pos[1];
	d[2] = pos[2];
	d[3] = height;
	d[4] = color[0];
	d[5] = color[1];
	d[6] = color[2];
	d[7] = falloff;
	d[8] = size;
	d[9] = dir ? dir[0] : 0.0f;
	d[10] = dir ? dir[1] : 0.0f;
	d[11] = dir ? dir[2] : 0.0f;
}

typedef struct
{
	int32_t key[3];
	double w, p[3], pp, n[3], e[3];
	int count;
} Cluster;

// open-addressed cells of size cell; returns index into cl (nc grows), -1 when full
static int clusterFor(Cluster *cl, int *nc, int maxc, int32_t *table, int tableSize, const float *pos, float cell)
{
	int32_t k[3] = {(int32_t)floorf(pos[0] / cell), (int32_t)floorf(pos[1] / cell), (int32_t)floorf(pos[2] / cell)};
	uint32_t h = ((uint32_t)k[0] * 73856093u ^ (uint32_t)k[1] * 19349663u ^ (uint32_t)k[2] * 83492791u) & (uint32_t)(tableSize - 1);
	for (int probe = 0; probe < tableSize; probe++)
	{
		int32_t *t = &table[(h + probe) & (uint32_t)(tableSize - 1)];
		if (*t < 0)
		{
			if (*nc >= maxc)
			{
				return -1;
			}
			*t = (*nc)++;
			memset(&cl[*t], 0, sizeof(Cluster));
			memcpy(cl[*t].key, k, sizeof(k));
			return *t;
		}
		if (!memcmp(cl[*t].key, k, sizeof(k)))
		{
			return *t;
		}
	}
	return -1;
}

static void clusterAdd(Cluster *c, const float *pos, const float *n, const float *e, double w)
{
	double sign = (c->n[0] * n[0] + c->n[1] * n[1] + c->n[2] * n[2] < 0.0) ? -1.0 : 1.0; // normals: sign per region is arbitrary
	for (int k = 0; k < 3; k++)
	{
		c->p[k] += pos[k] * w;
		c->n[k] += sign * n[k] * w;
		c->e[k] += e[k];
	}
	c->pp += (pos[0] * pos[0] + pos[1] * pos[1] + pos[2] * pos[2]) * w;
	c->w += w;
	c->count++;
}

static void clusterShape(const Cluster *c, float *centre, float *normal, float *radius)
{
	for (int k = 0; k < 3; k++)
	{
		centre[k] = (float)(c->p[k] / c->w);
	}
	double nl = sqrt(c->n[0] * c->n[0] + c->n[1] * c->n[1] + c->n[2] * c->n[2]);
	for (int k = 0; k < 3; k++)
	{
		normal[k] = (nl > 1e-9) ? (float)(c->n[k] / nl) : (k == 1 ? 1.0f : 0.0f);
	}
	double var = c->pp / c->w - (centre[0] * centre[0] + centre[1] * centre[1] + centre[2] * centre[2]);
	*radius = (float)sqrt(var > 0.0 ? var : 0.0);
}

// glowing level triangles (additive textures, animated ones, very bright opaque
// ones such as lamp bulbs) -> point lights per 300-unit cluster with the power
// they emit.
// Opaque glowing triangles are flagged emissive (shown as the PS1 draws them).
static void addGlowLights(const struct RtxLevelVert *verts, struct RtxLevelTri *tris, uint32_t nt, float *lightData)
{
	const struct RtxTexSlot *slots = (const void *)(R.shm + RTX_OFF_TEX_SLOTS);
	enum
	{
		MAXC = 1024,
		TABLE = 4096
	};
	static Cluster cl[MAXC];
	static int32_t table[TABLE];
	memset(table, 0xff, sizeof(table));
	int nc = 0, numTris = 0;
	for (uint32_t i = 0; i < nt; i++)
	{
		struct RtxLevelTri *t = &tris[i];
		if (!(t->flags & RTX_TRI_TEXTURED) || (t->flags & RTX_TRI_WATER) || (t->texSlot >= R.hdr->numTexSlots))
		{
			continue;
		}
		const struct RtxTexSlot *sl = &slots[t->texSlot];
		int blend = (sl->tpage >> 5) & 3;
		float tex[3] = {0, 0, 0}, rgb[3];
		int count = 0;
		for (int c = 0; c < 4; c++)
		{
			// the three corners and the middle of the triangle's texture
			int u = 0, v = 0;
			for (int k = 0; k < 3; k++)
			{
				const uint8_t *uv = sl->uv[t->corner[k]];
				u += (c == 3 || c == k) ? uv[0] : 0;
				v += (c == 3 || c == k) ? uv[1] : 0;
			}
			if (c == 3)
			{
				u /= 3;
				v /= 3;
			}
			if (cpuTexel(sl->tpage, sl->clut, u, v, rgb))
			{
				for (int k = 0; k < 3; k++)
				{
					tex[k] += rgb[k];
				}
				count++;
			}
		}
		if (count == 0)
		{
			continue;
		}
		float vcol[3] = {0, 0, 0}, ps1[3];
		for (int c = 0; c < 3; c++)
		{
			uint32_t rgba = verts[t->v[c]].rgba;
			vcol[0] += (rgba & 255) / 128.0f / 3.0f;
			vcol[1] += ((rgba >> 8) & 255) / 128.0f / 3.0f;
			vcol[2] += ((rgba >> 16) & 255) / 128.0f / 3.0f;
		}
		for (int k = 0; k < 3; k++)
		{
			ps1[k] = fminf(tex[k] / count * vcol[k], 1.0f);
		}
		float vmax = fmaxf(vcol[0], fmaxf(vcol[1], vcol[2]));
		float pmax = fmaxf(ps1[0], fmaxf(ps1[1], ps1[2])), pmin = fminf(ps1[0], fminf(ps1[1], ps1[2]));
		int vivid = (luma3(ps1) >= 0.08f) && (pmax - pmin >= 0.25f * pmax); // grey/white glows are sparkle, foam, highlights
		float strength;
		if ((blend == 1) && vivid)
		{
			strength = 1.0f; // additive: drawn as light
		}
		else if ((t->flags & RTX_TRI_ANIMATED) && vivid && (luma3(ps1) >= 0.2f))
		{
			// animated textures are CTR's glowing things: boosters, lava, flames, screens
			strength = 0.8f;
			if (blend == 3)
			{
				t->flags |= RTX_TRI_EMISSIVE;
			}
		}
		else if ((blend == 3) && (luma3(ps1) >= 0.85f) && (vmax >= 1.6f))
		{
			strength = 0.5f; // opaque, far brighter than neutral: lamp bulbs, lava
			t->flags |= RTX_TRI_EMISSIVE;
		}
		else
		{
			continue;
		}
		const float *p[3] = {verts[t->v[0]].pos, verts[t->v[1]].pos, verts[t->v[2]].pos};
		float fn[3];
		faceNormal(p[0], p[1], p[2], fn);
		float area = 0.5f * sqrtf(fn[0] * fn[0] + fn[1] * fn[1] + fn[2] * fn[2]);
		if (area < 1.0f)
		{
			continue;
		}
		float nrm[3] = {fn[0] / (2 * area), fn[1] / (2 * area), fn[2] / (2 * area)};
		float centre[3] = {(p[0][0] + p[1][0] + p[2][0]) / 3, (p[0][1] + p[1][1] + p[2][1]) / 3, (p[0][2] + p[1][2] + p[2][2]) / 3};
		// emitted power ~ radiance x area (half of it goes each way for glow sheets)
		float power[3];
		for (int k = 0; k < 3; k++)
		{
			power[k] = srgbLin(ps1[k]) * area * 0.5f * strength;
		}
		int c = clusterFor(cl, &nc, MAXC, table, TABLE, centre, 300.0f);
		if (c >= 0)
		{
			clusterAdd(&cl[c], centre, nrm, power, luma3(power) + 1e-6);
			numTris++;
		}
	}
	// strongest first, at most 384: the lamp pools need room too
	for (int a = 0; a < nc; a++)
	{
		for (int b = a + 1; b < nc; b++)
		{
			if (cl[b].w > cl[a].w)
			{
				Cluster t = cl[a];
				cl[a] = cl[b];
				cl[b] = t;
			}
		}
	}
	nc = nc < 384 ? nc : 384;
	int added = 0;
	for (int c = 0; (c < nc) && (R.numLights < RTX_MAX_LIGHTS); c++)
	{
		float centre[3], normal[3], radius;
		clusterShape(&cl[c], centre, normal, &radius);
		float falloff = fmaxf(200.0f, radius * 1.5f);
		float color[3];
		// point light with the cluster's intensity: f^2 * color = power / pi (far field)
		for (int k = 0; k < 3; k++)
		{
			color[k] = (float)cl[c].e[k] / (3.14159265f * falloff * falloff) * R.emissiveGain;
		}
		if (luma3(color) < 0.01f)
		{
			continue;
		}
		putLight(&lightData[R.numLights * 12], centre, 30.0f, color, falloff, fmaxf(10.0f, fminf(radius * 0.5f, 75.0f)), normal);
		R.numLights++;
		added++;
	}
	R.numGlowLights = (uint32_t)added;
	logf_("%d glowing triangles -> %d lights", numTris, added);
}

// icy levels: see-through bluish surfaces (ice walls, ice around the frozen
// polar bears) are ice too
static void markIce(const struct RtxLevelVert *verts, struct RtxLevelTri *tris, uint32_t nt)
{
	const struct RtxTexSlot *slots = (const void *)(R.shm + RTX_OFF_TEX_SLOTS);
	uint32_t ice = 0, added = 0;
	for (uint32_t i = 0; i < nt; i++)
	{
		ice += (tris[i].flags & RTX_TRI_ICE) != 0;
	}
	for (uint32_t i = 0; (ice > 0) && (i < nt); i++)
	{
		struct RtxLevelTri *t = &tris[i];
		if (!(t->flags & RTX_TRI_TEXTURED) || (t->flags & (RTX_TRI_WATER | RTX_TRI_ICE)) || (t->texSlot >= R.hdr->numTexSlots))
		{
			continue;
		}
		const struct RtxTexSlot *sl = &slots[t->texSlot];
		if (((sl->tpage >> 5) & 3) == 3)
		{
			continue; // opaque
		}
		float tex[3] = {0, 0, 0}, rgb[3];
		int count = 0;
		for (int c = 0; c < 3; c++)
		{
			const uint8_t *uv = sl->uv[t->corner[c]];
			if (cpuTexel(sl->tpage, sl->clut, uv[0], uv[1], rgb))
			{
				for (int k = 0; k < 3; k++)
				{
					tex[k] += rgb[k] / 3.0f;
				}
				count++;
			}
		}
		if ((count > 0) && (tex[2] >= tex[0] * 1.05f) && (luma3(tex) >= 0.25f))
		{
			t->flags |= RTX_TRI_ICE;
			added++;
		}
	}
	if (ice)
	{
		logf_("ice: %u triangles, %u see-through bluish ones added", ice, added);
	}
}

// lamp pools: vertices brighter than baseline (what the ambient/sun gives),
// clustered per 450 units into lights placed off the pool. Their colour is
// fitted after lights.comp has placed them (fitHotspots).
static int *s_hotCluster; // per vertex: hotspot light index, or -1
static float *s_hotTarget; // per vertex: excess light (luma)

static void addHotspots(const struct RtxLevelVert *verts, const uint8_t *used, uint32_t nv, const float *baseline, float *lightData)
{
	enum
	{
		MAXC = 1024,
		TABLE = 4096
	};
	static Cluster cl[MAXC];
	static int32_t table[TABLE];
	memset(table, 0xff, sizeof(table));
	free(s_hotCluster);
	free(s_hotTarget);
	s_hotCluster = malloc(sizeof(int) * (nv ? nv : 1));
	s_hotTarget = calloc(nv ? nv : 1, sizeof(float));
	int *cellOf = malloc(sizeof(int) * (nv ? nv : 1));
	int nc = 0;

	// threshold: well above the baseline and the median vertex
	float *lum = malloc(sizeof(float) * (nv ? nv : 1));
	int nu = 0;
	for (uint32_t i = 0; i < nv; i++)
	{
		float c[3];
		vertLinColor(&verts[i], c);
		if (used[i])
		{
			lum[nu++] = luma3(c);
		}
	}
	qsort(lum, nu, sizeof(float), cmpFloat);
	float median = nu ? lum[nu / 2] : 0.0f;
	free(lum);
	float threshold = fmaxf(fmaxf(2.0f * luma3(baseline), 2.0f * median), 0.2f);

	for (uint32_t i = 0; i < nv; i++)
	{
		s_hotCluster[i] = -1;
		cellOf[i] = -1;
		float c[3];
		vertLinColor(&verts[i], c);
		if (!used[i] || (luma3(c) < threshold))
		{
			continue;
		}
		float e[3] = {fmaxf(c[0] - baseline[0], 0.0f), fmaxf(c[1] - baseline[1], 0.0f), fmaxf(c[2] - baseline[2], 0.0f)};
		float w = luma3(e);
		int k = clusterFor(cl, &nc, MAXC, table, TABLE, verts[i].pos, 450.0f);
		if ((k < 0) || (w <= 0.0f))
		{
			continue;
		}
		clusterAdd(&cl[k], verts[i].pos, &R.levelNrm[i * 4], e, w);
		cellOf[i] = k;
		s_hotTarget[i] = w;
	}

	int *lightOf = malloc(sizeof(int) * MAXC);
	int added = 0;
	for (int k = 0; k < nc; k++)
	{
		lightOf[k] = -1;
		if ((cl[k].count < 3) || (cl[k].w < 0.3) || (R.numLights >= RTX_MAX_LIGHTS))
		{
			continue;
		}
		float centre[3], normal[3], radius;
		clusterShape(&cl[k], centre, normal, &radius);
		radius = fmaxf(80.0f, fminf(radius, 700.0f));
		double el = 0.299 * cl[k].e[0] + 0.587 * cl[k].e[1] + 0.114 * cl[k].e[2];
		float chroma[3] = {(float)(cl[k].e[0] / el), (float)(cl[k].e[1] / el), (float)(cl[k].e[2] / el)};
		// candidates at three heights off the pool: the shadowed fit (fitArt)
		// keeps the ones whose light and shadows match the art
		float height = fmaxf(80.0f, fminf(radius * 0.7f, 400.0f));
		static const float heights[3] = {0.5f, 1.0f, 2.0f};
		lightOf[k] = (int)R.numLights;
		for (int h = 0; (h < 3) && (R.numLights < RTX_MAX_LIGHTS); h++)
		{
			float c3[3] = {chroma[0] / 3.0f, chroma[1] / 3.0f, chroma[2] / 3.0f};
			putLight(&lightData[R.numLights * 12], centre, height * heights[h], c3, fmaxf(250.0f, radius * 1.4f) * (0.8f + 0.2f * h),
			         fmaxf(12.0f, fminf(radius * 0.125f, 60.0f)), normal);
			R.numLights++;
			added++;
		}
	}
	for (uint32_t i = 0; i < nv; i++)
	{
		s_hotCluster[i] = (cellOf[i] >= 0) ? lightOf[cellOf[i]] : -1;
	}
	free(lightOf);
	free(cellOf);
	logf_("lamp pools: threshold %.2f (median %.2f, baseline %.2f %.2f %.2f) -> %d lights", threshold, median, baseline[0], baseline[1], baseline[2], added);
}

// hotspot light colours (chroma x strength) so that the lights, as placed,
// reproduce the pools: non-negative least squares by multiplicative updates
// (s *= sum g t / sum g pred), each pool vertex seeing every hotspot light in
// reach (neighbouring pools overlap)
static void fitHotspots(const struct RtxLevelVert *verts, uint32_t nv, float *lightData, uint32_t first, uint32_t count)
{
	if (count == 0)
	{
		return;
	}
	float *scale = malloc(sizeof(float) * count);
	double *num = malloc(sizeof(double) * count), *den = malloc(sizeof(double) * count);
	uint32_t *hot = malloc(sizeof(uint32_t) * (nv ? nv : 1));
	uint32_t nh = 0;
	for (uint32_t i = 0; i < nv; i++)
	{
		if (s_hotCluster[i] >= 0)
		{
			hot[nh++] = i;
		}
	}
	for (uint32_t l = 0; l < count; l++)
	{
		scale[l] = 1.0f;
	}
	// each hot vertex's lights in reach and their (unscaled) light there, once:
	// the iterations then only visit those (dense, levels with 1000 lamp pools
	// took seconds)
	uint32_t *rowStart = malloc(sizeof(uint32_t) * (nh + 1));
	size_t cap = (size_t)nh * 8 + 64, nnz = 0;
	uint32_t *col = malloc(sizeof(uint32_t) * cap);
	float *gain = malloc(sizeof(float) * cap);
	for (uint32_t h = 0; h < nh; h++)
	{
		uint32_t i = hot[h];
		const float *p = verts[i].pos, *n = &R.levelNrm[i * 4];
		rowStart[h] = (uint32_t)nnz;
		for (uint32_t l = 0; l < count; l++)
		{
			const float *d = &lightData[(first + l) * 12];
			float dx = d[0] - p[0], dy = d[1] - p[1], dz = d[2] - p[2];
			float d2 = dx * dx + dy * dy + dz * dz;
			if (d2 > 25.0f * d[7] * d[7])
			{
				continue;
			}
			float inv = 1.0f / sqrtf(fmaxf(d2, 1.0f));
			float xw = d2 / (25.0f * d[7] * d[7]), win = fmaxf(1.0f - xw * xw, 0.0f);
			float g = fabsf(n[0] * dx + n[1] * dy + n[2] * dz) * inv * win * win / (1.0f + d2 / (d[7] * d[7])) * luma3(&d[4]); // as lightFalloff
			if (nnz == cap)
			{
				cap *= 2;
				col = realloc(col, sizeof(uint32_t) * cap);
				gain = realloc(gain, sizeof(float) * cap);
			}
			col[nnz] = l;
			gain[nnz] = g;
			nnz++;
		}
	}
	rowStart[nh] = (uint32_t)nnz;
	for (int iter = 0; iter < 6; iter++)
	{
		memset(num, 0, sizeof(double) * count);
		memset(den, 0, sizeof(double) * count);
		for (uint32_t h = 0; h < nh; h++)
		{
			// prediction from every hotspot light in reach, and each one's share
			double pred = 0.0;
			for (uint32_t k = rowStart[h]; k < rowStart[h + 1]; k++)
			{
				pred += scale[col[k]] * gain[k];
			}
			for (uint32_t k = rowStart[h]; k < rowStart[h + 1]; k++)
			{
				num[col[k]] += gain[k] * s_hotTarget[hot[h]];
				den[col[k]] += gain[k] * pred;
			}
		}
		for (uint32_t l = 0; l < count; l++)
		{
			if (den[l] > 1e-12)
			{
				scale[l] = (float)fmin(scale[l] * num[l] / den[l], 64.0);
			}
		}
	}
	free(rowStart);
	free(col);
	free(gain);
	for (uint32_t l = 0; l < count; l++)
	{
		float *d = &lightData[(first + l) * 12];
		d[4] *= scale[l];
		d[5] *= scale[l];
		d[6] *= scale[l];
	}
	free(scale);
	free(num);
	free(den);
	free(hot);
}

// cells over the level listing the static lights that reach them (5 x falloff)
static void buildLightGrid(const struct RtxLevelVert *verts, uint32_t nv, const float *lightData)
{
	float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
	for (uint32_t i = 0; i < nv; i++)
	{
		for (int k = 0; k < 3; k++)
		{
			lo[k] = fminf(lo[k], verts[i].pos[k]);
			hi[k] = fmaxf(hi[k], verts[i].pos[k]);
		}
	}
	if (nv == 0)
	{
		memset(lo, 0, sizeof(lo));
		memset(hi, 0, sizeof(hi));
	}
	float extent = fmaxf(hi[0] - lo[0], fmaxf(hi[1] - lo[1], hi[2] - lo[2]));
	uint32_t *g = R.lightGrid.map;
	for (float cell = fmaxf(extent / 40.0f, 600.0f);; cell *= 1.5f)
	{
		uint32_t dim[3];
		for (int k = 0; k < 3; k++)
		{
			dim[k] = (uint32_t)((hi[k] - lo[k]) / cell) + 1;
		}
		uint32_t cells = dim[0] * dim[1] * dim[2];
		if (cells * 2 > LIGHT_GRID_WORDS / 2)
		{
			continue;
		}
		// count, then fill
		memset(g, 0, cells * 2 * sizeof(uint32_t));
		uint32_t total = 0;
		for (int pass = 0; pass < 2; pass++)
		{
			for (uint32_t l = 0; l < R.numLights; l++)
			{
				const float *d = &lightData[l * 12];
				float reach = 5.0f * d[7];
				int c0[3], c1[3];
				for (int k = 0; k < 3; k++)
				{
					c0[k] = (int)floorf((d[k] - reach - lo[k]) / cell);
					c1[k] = (int)floorf((d[k] + reach - lo[k]) / cell);
					c0[k] = c0[k] < 0 ? 0 : c0[k];
					c1[k] = c1[k] >= (int)dim[k] ? (int)dim[k] - 1 : c1[k];
				}
				for (int z = c0[2]; z <= c1[2]; z++)
				for (int y = c0[1]; y <= c1[1]; y++)
				for (int x = c0[0]; x <= c1[0]; x++)
				{
					uint32_t c = ((uint32_t)z * dim[1] + (uint32_t)y) * dim[0] + (uint32_t)x;
					if (pass == 0)
					{
						g[c * 2 + 1]++;
						total++;
					}
					else
					{
						g[g[c * 2] + g[c * 2 + 1]++] = l;
					}
				}
			}
			if (pass == 0)
			{
				if (cells * 2 + total > LIGHT_GRID_WORDS)
				{
					break; // too many: bigger cells
				}
				uint32_t offset = cells * 2;
				for (uint32_t c = 0; c < cells; c++)
				{
					g[c * 2] = offset;
					offset += g[c * 2 + 1];
					g[c * 2 + 1] = 0;
				}
			}
		}
		if (cells * 2 + total > LIGHT_GRID_WORDS)
		{
			continue;
		}
		memcpy(R.gridMin, lo, sizeof(lo));
		R.gridMin[3] = cell;
		memcpy(R.gridDim, dim, sizeof(dim));
		logf_("light grid %ux%ux%u cells of %.0f, %u entries", dim[0], dim[1], dim[2], cell, total);
		return;
	}
}

// ---- level ---------------------------------------------------------------------------

// Neighbouring quadblocks keep their own copies of a shared corner, so their
// normals only average within one block and shading steps at every block edge,
// even shallow ones. Copies at the same position share normals here when they
// are within ~70 degrees (dune ridges smooth; a wall meeting the floor stays sharp)
static void weldLevelNormals(const struct RtxLevelVert *verts, uint32_t nv, float *nrm)
{
	enum
	{
		HASH = 1 << 17
	};
	int32_t *head = malloc(sizeof(int32_t) * HASH), *next = malloc(sizeof(int32_t) * (nv ? nv : 1));
	float *out = malloc(sizeof(float) * 4 * (nv ? nv : 1));
	memset(head, 0xff, sizeof(int32_t) * HASH);
	for (uint32_t i = 0; i < nv; i++)
	{
		int32_t x = (int32_t)lrintf(verts[i].pos[0]), y = (int32_t)lrintf(verts[i].pos[1]), z = (int32_t)lrintf(verts[i].pos[2]);
		uint32_t h = ((uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)z * 83492791u) & (HASH - 1);
		next[i] = head[h];
		head[h] = (int32_t)i;
	}
	for (uint32_t i = 0; i < nv; i++)
	{
		const float *a = &nrm[i * 4];
		float la = sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
		float *o = &out[i * 4];
		memcpy(o, a, sizeof(float) * 4);
		if (la < 1e-12f)
		{
			continue;
		}
		int32_t x = (int32_t)lrintf(verts[i].pos[0]), y = (int32_t)lrintf(verts[i].pos[1]), z = (int32_t)lrintf(verts[i].pos[2]);
		uint32_t h = ((uint32_t)x * 73856093u ^ (uint32_t)y * 19349663u ^ (uint32_t)z * 83492791u) & (HASH - 1);
		for (int32_t j = head[h]; j >= 0; j = next[j])
		{
			if (((uint32_t)j == i) || (lrintf(verts[j].pos[0]) != x) || (lrintf(verts[j].pos[1]) != y) || (lrintf(verts[j].pos[2]) != z))
			{
				continue;
			}
			const float *b = &nrm[j * 4];
			float lb = sqrtf(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
			if ((lb > 1e-12f) && ((a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) > 0.34f * la * lb))
			{
				o[0] += b[0];
				o[1] += b[1];
				o[2] += b[2];
			}
		}
	}
	memcpy(nrm, out, sizeof(float) * 4 * nv);
	free(head);
	free(next);
	free(out);
}

// The level's triangles come out of the quadblocks' draw tables wound either way
// (mirrored faces flip), so a plain sum of face normals cancels between
// neighbours and smooth shading breaks up along polygon edges. levelOrient gives
// every triangle a sign that makes its normal agree with its neighbours (spread
// across shared edges, not across folds sharper than ~80 degrees), each connected
// patch facing up overall (floors are seen from above). Patches with nothing
// near horizontal (walls apart from their floor) can't tell: sign +-2.
static int cmpInts(const int32_t *x, const int32_t *y, int n)
{
	for (int k = 0; k < n; k++)
	{
		if (x[k] != y[k])
		{
			return (x[k] > y[k]) - (x[k] < y[k]);
		}
	}
	return 0;
}

static int cmpEdgeRec(const void *a, const void *b) { return cmpInts(a, b, 6); }
static int cmpCornerKey(const void *a, const void *b) { return cmpInts(a, b, 3); }

static void levelPosKey(const struct RtxLevelVert *v, int32_t *k)
{
	for (int i = 0; i < 3; i++)
	{
		k[i] = (int32_t)lrintf(v->pos[i]);
	}
}

static void levelOrient(const struct RtxLevelVert *verts, const struct RtxLevelTri *tris, uint32_t nt, const float *fu, const float *fn, int8_t *sign)
{
	// edge records: lower key, higher key, triangle, forward (lower -> higher in its winding)
	int32_t *rec = malloc(sizeof(int32_t) * 8 * 3 * (nt ? nt : 1));
	uint32_t ne = 0;
	for (uint32_t t = 0; t < nt; t++)
	{
		for (int e = 0; e < 3; e++)
		{
			int32_t ka[3], kb[3];
			levelPosKey(&verts[tris[t].v[e]], ka);
			levelPosKey(&verts[tris[t].v[(e + 1) % 3]], kb);
			int fwd = cmpInts(ka, kb, 3) < 0;
			int32_t *r = &rec[ne++ * 8];
			memcpy(r, fwd ? ka : kb, 12);
			memcpy(r + 3, fwd ? kb : ka, 12);
			r[6] = (int32_t)t;
			r[7] = fwd;
		}
	}
	qsort(rec, ne, sizeof(int32_t) * 8, cmpEdgeRec);
	// neighbours across manifold edges: (other triangle, winds the same way -> flip)
	int32_t *adj = malloc(sizeof(int32_t) * 2 * 3 * (nt ? nt : 1));
	uint8_t *numAdj = calloc(nt ? nt : 1, 1);
	for (uint32_t a = 0; a < ne;)
	{
		uint32_t b = a;
		while ((b < ne) && (cmpEdgeRec(&rec[a * 8], &rec[b * 8]) == 0))
		{
			b++;
		}
		if (b - a == 2)
		{
			uint32_t t0 = (uint32_t)rec[a * 8 + 6], t1 = (uint32_t)rec[(a + 1) * 8 + 6];
			float d = fu[t0 * 3] * fu[t1 * 3] + fu[t0 * 3 + 1] * fu[t1 * 3 + 1] + fu[t0 * 3 + 2] * fu[t1 * 3 + 2];
			if ((t0 != t1) && (fabsf(d) >= 0.17f) && (numAdj[t0] < 3) && (numAdj[t1] < 3))
			{
				int32_t same = rec[a * 8 + 7] == rec[(a + 1) * 8 + 7];
				adj[(t0 * 3 + numAdj[t0]) * 2] = (int32_t)t1;
				adj[(t0 * 3 + numAdj[t0]++) * 2 + 1] = same;
				adj[(t1 * 3 + numAdj[t1]) * 2] = (int32_t)t0;
				adj[(t1 * 3 + numAdj[t1]++) * 2 + 1] = same;
			}
		}
		a = b;
	}
	memset(sign, 0, nt);
	uint32_t *stack = malloc(sizeof(uint32_t) * (nt ? nt : 1)), *members = malloc(sizeof(uint32_t) * (nt ? nt : 1));
	uint32_t patches = 0, flipped = 0, unknown = 0;
	for (uint32_t s = 0; s < nt; s++)
	{
		if (sign[s])
		{
			continue;
		}
		uint32_t sp = 0, nm = 0;
		sign[s] = 1;
		stack[sp++] = s;
		members[nm++] = s;
		while (sp > 0)
		{
			uint32_t t = stack[--sp];
			for (int i = 0; i < numAdj[t]; i++)
			{
				uint32_t u = (uint32_t)adj[(t * 3 + i) * 2];
				int8_t want = adj[(t * 3 + i) * 2 + 1] ? (int8_t)-sign[t] : sign[t];
				if (sign[u] == 0)
				{
					sign[u] = want;
					stack[sp++] = u;
					members[nm++] = u;
				}
			}
		}
		double up = 0.0;
		for (uint32_t i = 0; i < nm; i++)
		{
			uint32_t t = members[i];
			if (fabsf(fu[t * 3 + 1]) > 0.7f)
			{
				up += fn[t * 3 + 1] * sign[t];
			}
		}
		int8_t m = (up < 0.0) ? -1 : 1;
		m = (fabs(up) < 1e-6) ? (int8_t)(m * 2) : m;
		for (uint32_t i = 0; i < nm; i++)
		{
			sign[members[i]] = (int8_t)(sign[members[i]] * m);
		}
		patches++;
	}
	for (uint32_t t = 0; t < nt; t++)
	{
		flipped += sign[t] < 0;
		unknown += abs(sign[t]) == 2;
	}
	logf_("level winding: %u patches, %u of %u triangles flipped to agree, %u facing unknown", patches, flipped, nt, unknown);
	free(rec);
	free(adj);
	free(numAdj);
	free(stack);
	free(members);
}

// Shading normals per triangle corner: at each position (shared vertex or a
// copy) the faces within 45 degrees of each other, chained, form one smoothing
// group with one normal (area weighted). A floor corner isn't tilted by the wall
// or step it touches, and both sides of a smooth edge get the same normal
static void cornerNormals(const struct RtxLevelVert *verts, const struct RtxLevelTri *tris, uint32_t nt, const float *fu, const float *fn, float *out)
{
	int32_t *key = malloc(sizeof(int32_t) * 4 * 3 * (nt ? nt : 1)); // x, y, z, corner
	for (uint32_t i = 0; i < nt; i++)
	{
		for (int c = 0; c < 3; c++)
		{
			int32_t *e = &key[(i * 3 + c) * 4];
			levelPosKey(&verts[tris[i].v[c]], e);
			e[3] = (int32_t)(i * 3 + c);
		}
	}
	qsort(key, (size_t)nt * 3, sizeof(int32_t) * 4, cmpCornerKey);
	uint32_t cap = 64;
	uint32_t *parent = malloc(sizeof(uint32_t) * cap);
	float *sum = malloc(sizeof(float) * 3 * cap);
	for (uint32_t a = 0; a < nt * 3;)
	{
		uint32_t b = a;
		while ((b < nt * 3) && (cmpCornerKey(&key[a * 4], &key[b * 4]) == 0))
		{
			b++;
		}
		uint32_t n = b - a;
		if (n > cap)
		{
			cap = n;
			parent = realloc(parent, sizeof(uint32_t) * cap);
			sum = realloc(sum, sizeof(float) * 3 * cap);
		}
		for (uint32_t i = 0; i < n; i++)
		{
			parent[i] = i;
		}
		for (uint32_t i = 0; i < n; i++)
		{
			uint32_t ti = (uint32_t)key[(a + i) * 4 + 3] / 3;
			for (uint32_t j = i + 1; j < n; j++)
			{
				uint32_t tj = (uint32_t)key[(a + j) * 4 + 3] / 3;
				float d = fu[ti * 3] * fu[tj * 3] + fu[ti * 3 + 1] * fu[tj * 3 + 1] + fu[ti * 3 + 2] * fu[tj * 3 + 2];
				if (d > 0.707f)
				{
					uint32_t x = i, y = j;
					while (parent[x] != x)
					{
						x = parent[x];
					}
					while (parent[y] != y)
					{
						y = parent[y];
					}
					parent[x] = y;
				}
			}
		}
		memset(sum, 0, sizeof(float) * 3 * n);
		for (uint32_t i = 0; i < n; i++)
		{
			uint32_t r = i, ti = (uint32_t)key[(a + i) * 4 + 3] / 3;
			while (parent[r] != r)
			{
				r = parent[r];
			}
			parent[i] = r;
			for (int k = 0; k < 3; k++)
			{
				sum[r * 3 + k] += fn[ti * 3 + k];
			}
		}
		for (uint32_t i = 0; i < n; i++)
		{
			float *o = &out[key[(a + i) * 4 + 3] * 4];
			memcpy(o, &sum[parent[i] * 3], 12);
			o[3] = 0.0f;
		}
		a = b;
	}
	free(parent);
	free(sum);
	free(key);
}

static void loadLevel(void)
{
	vkDeviceWaitIdle(R.dev);
	bufDestroy(&R.levelVerts);
	bufDestroy(&R.levelTris);
	bufDestroy(&R.levelIdx);
	bufDestroy(&R.levelNormals);
	accelDestroy(&R.levelBlas);

	uint32_t nv = R.hdr->numLevelVerts, nt = R.hdr->numLevelTris;
	const struct RtxLevelVert *verts = (const void *)(R.shm + RTX_OFF_LEVEL_VERTS);
	const struct RtxLevelTri *tris = (const void *)(R.shm + RTX_OFF_LEVEL_TRIS);
	uint32_t *idx = malloc(sizeof(uint32_t) * 3 * (nt ? nt : 1));
	for (uint32_t i = 0; i < nt; i++)
	{
		for (int c = 0; c < 3; c++)
		{
			idx[i * 3 + c] = tris[i].v[c];
		}
	}
	R.levelVerts = uploadBuf(verts, sizeof(*verts) * nv, USAGE_AS_INPUT);
	R.levelTris = uploadBuf(tris, sizeof(*tris) * nt, 0); // flags updated by levelPrep (emissive)
	R.levelIdx = uploadBuf(idx, sizeof(uint32_t) * 3 * nt, USAGE_AS_INPUT);
	free(idx);

	// face normals (area weighted), turned so neighbours agree (levelOrient)
	float *fn = malloc(sizeof(float) * 3 * (nt ? nt : 1)), *fu = malloc(sizeof(float) * 3 * (nt ? nt : 1));
	int8_t *orient = malloc(nt ? nt : 1);
	for (uint32_t i = 0; i < nt; i++)
	{
		faceNormal(verts[tris[i].v[0]].pos, verts[tris[i].v[1]].pos, verts[tris[i].v[2]].pos, &fn[i * 3]);
		float l = sqrtf(fn[i * 3] * fn[i * 3] + fn[i * 3 + 1] * fn[i * 3 + 1] + fn[i * 3 + 2] * fn[i * 3 + 2]);
		for (int k = 0; k < 3; k++)
		{
			fu[i * 3 + k] = (l > 1e-9f) ? fn[i * 3 + k] / l : 0.0f;
		}
	}
	levelOrient(verts, tris, nt, fu, fn, orient);
	for (uint32_t i = 0; i < nt * 3; i++)
	{
		fn[i] *= (orient[i / 3] > 0) ? 1.0f : -1.0f;
		fu[i] *= (orient[i / 3] > 0) ? 1.0f : -1.0f;
	}
	free(R.levelNrm);
	R.levelNrm = calloc((size_t)(nv ? nv : 1) * 4, sizeof(float));
	for (uint32_t i = 0; i < nt; i++)
	{
		for (int c = 0; c < 3; c++)
		{
			float *a = &R.levelNrm[tris[i].v[c] * 4];
			a[0] += fn[i * 3];
			a[1] += fn[i * 3 + 1];
			a[2] += fn[i * 3 + 2];
		}
	}
	weldLevelNormals(verts, nv, R.levelNrm);
	// per vertex (bake.comp), then per corner (shading: fetchTriangle; they also
	// tell the oriented front side, see levelBackFacing)
	float *nrmAll = malloc(sizeof(float) * 4 * ((size_t)nv + (size_t)nt * 3 + 1));
	memcpy(nrmAll, R.levelNrm, sizeof(float) * 4 * nv);
	cornerNormals(verts, tris, nt, fu, fn, &nrmAll[(size_t)nv * 4]);
	for (uint32_t i = 0; i < nt; i++)
	{
		nrmAll[((size_t)nv + i * 3) * 4 + 3] = (abs(orient[i]) == 1) ? 1.0f : 0.0f; // w: front side known
	}
	R.cornerBase = nv;
	R.levelFront = 1.0f;
	R.levelNormals = uploadBuf(nrmAll, sizeof(float) * 4 * ((size_t)nv + (size_t)nt * 3 + 1), 0);
	free(nrmAll);
	free(fn);
	free(fu);
	free(orient);
	for (uint32_t i = 0; i < nv; i++)
	{
		float *n = &R.levelNrm[i * 4];
		float l = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		for (int k = 0; k < 3; k++)
		{
			n[k] = (l > 1e-12f) ? n[k] / l : (k == 1 ? 1.0f : 0.0f);
		}
	}

	cmdBegin();
	VkAccelerationStructureGeometryKHR g = triGeometry(R.levelVerts.addr, sizeof(struct RtxLevelVert), nv ? nv - 1 : 0, R.levelIdx.addr);
	buildBlas(&R.levelBlas, &g, nt, nt, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	// a new level: the radiance cache starts empty
	vkCmdFillBuffer(R.cmd, R.cacheKeys.buf, 0, VK_WHOLE_SIZE, 0);
	vkCmdFillBuffer(R.cmd, R.cacheAccum.buf, 0, VK_WHOLE_SIZE, 0);
	vkCmdFillBuffer(R.cmd, R.cacheVal.buf, 0, VK_WHOLE_SIZE, 0);
	cmdSubmitWait();

	loadLighting(R.hdr->lightsKey);
	R.numLights = 0;
	R.gridDim[0] = R.gridDim[1] = R.gridDim[2] = 0;
	R.levelPrep = 1;
	R.haveLevel = 1;
	R.havePrev = 0;
	R.autoExposure = 0.0f;
	R.levelGen = R.hdr->levelGen;
	logf_("level %d: %u verts, %u tris", R.hdr->levelID, nv, nt);
}

// TLAS holding only the level (level prep passes trace against it)
static void buildLevelOnlyTlas(void)
{
	VkAccelerationStructureInstanceKHR inst = {0};
	inst.transform.matrix[0][0] = inst.transform.matrix[1][1] = inst.transform.matrix[2][2] = 1.0f;
	inst.mask = 0x1;
	inst.accelerationStructureReference = R.levelBlas.addr;
	memcpy(R.tlasInstances.map, &inst, sizeof(inst));
	VkAccelerationStructureGeometryKHR g = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR};
	g.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	g.geometry.instances.data.deviceAddress = R.tlasInstances.addr;
	VkAccelerationStructureBuildGeometryInfoKHR bi = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	                                                   .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	                                                   .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	                                                   .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	                                                   .dstAccelerationStructure = R.tlas.as,
	                                                   .geometryCount = 1,
	                                                   .pGeometries = &g};
	uint32_t one = 1;
	VkAccelerationStructureBuildSizesInfoKHR sizes = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	vkGetAccelerationStructureBuildSizesKHR(R.dev, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &bi, &one, &sizes);
	bi.scratchData.deviceAddress = scratchAddr(sizes.buildScratchSize);
	VkAccelerationStructureBuildRangeInfoKHR range = {.primitiveCount = 1};
	const VkAccelerationStructureBuildRangeInfoKHR *pr = &range;
	vkCmdBuildAccelerationStructuresKHR(R.cmd, 1, &bi, &pr);
	barrier(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT);
}

static void dispatchOnce(VkPipeline pipe, uint32_t groups, int step)
{
	vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
	vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &R.sets[0][PASS_TRACE], 0, NULL);
	vkCmdPushConstants(R.cmd, R.pipeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4, &step);
	vkCmdDispatch(R.cmd, groups ? groups : 1, 1, 1);
}

// sun from the vertex colours (sunfit.comp); returns 0 when no direction explains them
static int fitSun(const struct RtxLevelVert *verts, const uint8_t *used, uint32_t nv, float *baseline)
{
	float *fit = R.fit.map;
	uint32_t numUsed = 0;
	for (uint32_t i = 0; i < nv; i++)
	{
		numUsed += used[i];
	}
	uint32_t stride = numUsed > FIT_SAMPLES ? (numUsed + FIT_SAMPLES - 1) / FIT_SAMPLES : 1, ns = 0, k = 0;
	for (uint32_t i = 0; (i < nv) && (ns < FIT_SAMPLES); i++)
	{
		if (!used[i] || (k++ % stride))
		{
			continue;
		}
		float *s = &fit[(1 + ns * 3) * 4];
		memcpy(s, verts[i].pos, 12);
		s[3] = 0.0f;
		memcpy(&s[4], &R.levelNrm[i * 4], 12);
		s[7] = 0.0f;
		vertLinColor(&verts[i], &s[8]);
		s[11] = luma3(&s[8]);
		ns++;
	}
	fit[0] = (float)ns;
	fit[1] = (float)FIT_DIRS;
	cmdBegin();
	buildLevelOnlyTlas();
	dispatchOnce(R.pipeSunfit, (ns + 63) / 64, 0);
	barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
	dispatchOnce(R.pipeSunfit, (FIT_DIRS + 63) / 64, 1);
	cmdSubmitWait();

	double n = ns, bestR = -1.0;
	int best = -1;
	for (uint32_t d = 0; d < FIT_DIRS; d++)
	{
		const float *r = &fit[(1 + ns * 3 + d * 3) * 4];
		double sx = r[0], sxx = r[1], sl = r[2], sll = r[3], sxl = r[7];
		double den = n * sxx - sx * sx, denL = n * sll - sl * sl;
		if ((den < 1e-6 * n * n) || (denL <= 0.0))
		{
			continue;
		}
		double corr = (n * sxl - sx * sl) / sqrt(den * denL);
		// not near overhead: flat ground facing the sky correlates with "up" without
		// any sun, and an overhead sun hides every shadow under its caster
		float dy = 0.12f + (1.0f - 0.12f) * ((float)d + 0.5f) / (float)FIT_DIRS;
		if ((dy < 0.26f) || (dy > 0.85f))
		{
			continue;
		}
		if (corr > bestR)
		{
			bestR = corr;
			best = (int)d;
		}
	}
	if ((best < 0) || (bestR < 0.15))
	{
		logf_("sun fit: no direction explains the vertex colours (best r %.2f): no sun", bestR);
		return 0;
	}
	const float *r = &fit[(1 + ns * 3 + best * 3) * 4];
	double sx = r[0], sxx = r[1], den = n * sxx - sx * sx;
	float a[3], b[3];
	for (int c = 0; c < 3; c++)
	{
		double sy = r[4 + c], sxy = r[8 + c];
		b[c] = (float)fmax((n * sxy - sx * sy) / den, 0.0);
		a[c] = (float)fmax((sy - b[c] * sx) / n, 0.0);
	}
	struct RtxLighting *L = &R.light;
	float y = 0.12f + (1.0f - 0.12f) * ((float)best + 0.5f) / (float)FIT_DIRS, rr = sqrtf(fmaxf(1.0f - y * y, 0.0f)), phi = (float)best * 2.39996323f;
	L->sunDir[0] = rr * cosf(phi);
	L->sunDir[1] = y;
	L->sunDir[2] = rr * sinf(phi);
	for (int c = 0; c < 3; c++)
	{
		L->sunColor[c] = b[c];
		L->skyZenith[c] = a[c] * 1.15f;
		L->skyHorizon[c] = a[c] * 0.9f;
		L->ground[c] = a[c] * 0.35f;
		baseline[c] = a[c] + b[c];
	}
	logf_("sun fit (r %.2f, %u samples): dir %.2f %.2f %.2f, sun %.2f %.2f %.2f, sky %.2f %.2f %.2f", bestR, ns, L->sunDir[0], L->sunDir[1], L->sunDir[2],
	      b[0], b[1], b[2], a[0], a[1], a[2]);
	return 1;
}


// lighting constants for passes that shade (bake.comp), as renderFrame sets them
static void prepLightingConstants(FrameData *F)
{
	struct RtxLighting *L = &R.light;
	float sunLen = sqrtf(L->sunDir[0] * L->sunDir[0] + L->sunDir[1] * L->sunDir[1] + L->sunDir[2] * L->sunDir[2]);
	for (int i = 0; i < 3; i++)
	{
		F->sunDir[i] = L->sunDir[i] / (sunLen > 0 ? sunLen : 1);
		F->sunColor[i] = L->indoor ? 0.0f : L->sunColor[i];
		F->skyZenith[i] = L->skyZenith[i];
		F->skyHorizon[i] = L->skyHorizon[i];
		F->groundColor[i] = L->ground[i];
		F->ambient[i] = L->ambient[i];
	}
	F->sunDir[3] = L->sunCone;
	F->ambient[3] = L->indoor ? 1.0f : 0.0f;
	F->params[0] = 1500.0f;
	for (int i = 0; i < 4; i++)
	{
		F->gridMin[i] = R.gridMin[i];
	}
	for (int i = 0; i < 3; i++)
	{
		F->gridDim[i] = R.gridDim[i];
	}
	F->gridDim[3] = R.skyValid;
	memcpy(F->sky, R.sky, sizeof(F->sky));
}

#define BAKE_K 48u
#define BAKE_STRIDE (8u + 2u * BAKE_K)
#define BAKE_LIGHTS 8u // first (light, contribution) pair

// The art's lighting, fitted with shadows: bake.comp gives every vertex's
// shadowed light from the sun, the sky and each static light at unit strength;
// non-negative least squares (multiplicative updates) scales them to the
// vertex colours, so lights the art doesn't support fade out and the ones that
// cast the art's shadows stay. What the fitted light still can't explain is the
// art's own colour: kept per vertex as the surface tint (chroma fully, a part
// of the brightness), and as glow where the art is far brighter than any light.
static void fitArt(const struct RtxLevelVert *verts, const struct RtxLevelTri *tris, uint32_t nv, uint32_t nt, const uint8_t *used, float *lightData)
{
	FrameData *F = R.frame.map;
	prepLightingConstants(F);
	F->counts[0] = R.numLights;
	F->counts[1] = nv;
	cmdBegin();
	copyBuf(&R.lightsStage, &R.lights, (VkDeviceSize)R.numLights * 48);
	buildLevelOnlyTlas();
	barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
	dispatchOnce(R.pipeBake, (nv + 63) / 64, 0);
	cmdSubmitWait();
	const float *bake = R.bake.map;

	// vertices that glow on their own don't say anything about the light
	uint8_t *fitV = calloc(nv ? nv : 1, 1);
	for (uint32_t i = 0; i < nv; i++)
	{
		fitV[i] = used[i];
	}
	for (uint32_t i = 0; i < nt; i++)
	{
		if (tris[i].flags & (RTX_TRI_EMISSIVE | RTX_TRI_ANIMATED))
		{
			for (int c = 0; c < 3; c++)
			{
				fitV[tris[i].v[c]] = 0;
			}
		}
	}

	struct RtxLighting *L = &R.light;
	uint32_t np = 2 + R.numLights;
	double *scale = malloc(sizeof(double) * np), *num = malloc(sizeof(double) * np), *den = malloc(sizeof(double) * np);
	float *lightLum = malloc(sizeof(float) * (R.numLights ? R.numLights : 1));
	for (uint32_t p = 0; p < np; p++)
	{
		scale[p] = 1.0;
	}
	for (uint32_t l = 0; l < R.numLights; l++)
	{
		lightLum[l] = luma3(&lightData[l * 12 + 4]);
	}
	float sunLum = L->indoor ? 0.0f : luma3(L->sunColor);
	double err0 = 0.0, err1 = 0.0;
	uint32_t nfit = 0;
	for (int iter = 0; iter <= 40; iter++)
	{
		memset(num, 0, sizeof(double) * np);
		memset(den, 0, sizeof(double) * np);
		double err = 0.0;
		for (uint32_t v = 0; v < nv; v++)
		{
			if (!fitV[v])
			{
				continue;
			}
			const float *b = &bake[v * BAKE_STRIDE];
			float c[3];
			vertLinColor(&verts[v], c);
			double t = luma3(c);
			double bounce = luma3(&b[4]); // fixed: the art's own brightness, bounced
			double pred = bounce + scale[0] * b[0] * sunLum + scale[1] * luma3(&b[1]);
			for (uint32_t k = 0; k < BAKE_K; k++)
			{
				uint32_t j;
				memcpy(&j, &b[BAKE_LIGHTS + k * 2], 4);
				if (j == 0)
				{
					break;
				}
				pred += scale[2 + j - 1] * b[BAKE_LIGHTS + 1 + k * 2] * lightLum[j - 1];
			}
			err += fabs(log((pred + 0.02) / (t + 0.02)));
			num[0] += b[0] * sunLum * t;
			den[0] += b[0] * sunLum * pred;
			num[1] += luma3(&b[1]) * t;
			den[1] += luma3(&b[1]) * pred;
			for (uint32_t k = 0; k < BAKE_K; k++)
			{
				uint32_t j;
				memcpy(&j, &b[BAKE_LIGHTS + k * 2], 4);
				if (j == 0)
				{
					break;
				}
				double a = b[BAKE_LIGHTS + 1 + k * 2] * lightLum[j - 1];
				num[2 + j - 1] += a * t;
				den[2 + j - 1] += a * pred;
			}
			if (iter == 0)
			{
				nfit++;
			}
		}
		if (iter == 0)
		{
			err0 = err;
		}
		err1 = err;
		if (iter == 40)
		{
			break;
		}
		for (uint32_t p = 0; p < np; p++)
		{
			if (den[p] > 1e-12)
			{
				scale[p] = fmin(scale[p] * num[p] / den[p], 50.0);
			}
		}
	}
	// file lights keep their strength
	for (uint32_t l = 0; l < R.numFileLights; l++)
	{
		scale[2 + l] = 1.0;
	}

	// fitted strengths -> the lighting
	for (int k = 0; k < 3; k++)
	{
		L->sunColor[k] *= (float)scale[0];
		L->skyZenith[k] *= (float)scale[1];
		L->skyHorizon[k] *= (float)scale[1];
		L->ground[k] *= (float)scale[1];
		L->ambient[k] *= (float)scale[1];
	}
	for (int i = 0; i < 64; i++)
	{
		for (int k = 0; k < 3; k++)
		{
			R.sky[i][k] *= (float)scale[1];
		}
	}
	for (uint32_t l = 0; l < R.numLights; l++)
	{
		for (int k = 0; k < 3; k++)
		{
			lightData[l * 12 + 4 + k] *= (float)scale[2 + l];
		}
	}
	// outdoors the sun keeps at least some say, even where the art is lit evenly
	// (snow, overcast): shadows under things are the point of tracing
	float sunMin = R.sunMin * luma3(L->skyHorizon) / 0.9f;
	if (!L->indoor && (luma3(L->sunColor) < sunMin))
	{
		float lum = luma3(L->sunColor);
		for (int k = 0; k < 3; k++)
		{
			L->sunColor[k] = (lum > 1e-4f) ? L->sunColor[k] * sunMin / lum : sunMin;
		}
		logf_("sun raised to %.2f of the sky (the art alone gives %.2f)", R.sunMin, lum / fmaxf(luma3(L->skyHorizon) / 0.9f, 1e-4f));
	}

	// the art's tint per vertex. Its colour is taken against the level's average
	// colour (the overall cast is the lights' job): only what the artists painted
	// locally stays in the surface. Against each vertex's predicted light it would
	// paint complementary colours wherever the traced light differs (pink snow)
	float *tint = R.levelTintStage.map;
	uint32_t glowing = 0;
	double mean[3] = {0, 0, 0};
	for (uint32_t v = 0; v < nv; v++)
	{
		float c[3];
		vertLinColor(&verts[v], c);
		for (int k = 0; (k < 3) && used[v]; k++)
		{
			mean[k] += c[k];
		}
	}
	float meanC[3] = {(float)mean[0], (float)mean[1], (float)mean[2]};
	float meanL = luma3(meanC) + 1e-6f;
	for (uint32_t v = 0; v < nv; v++)
	{
		float *o = &tint[v * 4];
		o[0] = o[1] = o[2] = 1.0f;
		o[3] = 0.0f;
		if (!used[v])
		{
			continue;
		}
		const float *b = &bake[v * BAKE_STRIDE];
		float pred[3];
		for (int k = 0; k < 3; k++)
		{
			pred[k] = b[0] * (L->indoor ? 0.0f : L->sunColor[k]) + b[1 + k] * (float)scale[1] + b[4 + k];
		}
		for (uint32_t k = 0; k < BAKE_K; k++)
		{
			uint32_t j;
			memcpy(&j, &b[BAKE_LIGHTS + k * 2], 4);
			if (j == 0)
			{
				break;
			}
			for (int ch = 0; ch < 3; ch++)
			{
				pred[ch] += b[BAKE_LIGHTS + 1 + k * 2] * lightData[(j - 1) * 12 + 4 + ch];
			}
		}
		float c[3];
		vertLinColor(&verts[v], c);
		float lc = luma3(c), lp = luma3(pred);
		float ratio = (lc + 0.01f) / (lp + 0.01f);
		float bright = fminf(fmaxf(powf(ratio, R.tintLum), 0.5f), 2.0f);
		for (int k = 0; k < 3; k++)
		{
			float chroma = ((c[k] + 0.005f) / (lc + 0.005f)) / ((meanC[k] + 1e-6f) / meanL);
			o[k] = fminf(fmaxf(powf(chroma, R.tintChroma), 0.4f), 2.5f) * bright;
		}
		// keep the tint's brightness in check: colour, not light
		float lt = luma3(o);
		if (lt > 2.0f)
		{
			for (int k = 0; k < 3; k++)
			{
				o[k] *= 2.0f / lt;
			}
		}
		if ((lc > 1.0f) && (ratio > 3.0f))
		{
			o[3] = fminf((ratio - 3.0f) / 3.0f, 1.0f);
			glowing++;
		}
	}
	cmdBegin();
	copyBuf(&R.levelTintStage, &R.levelTint, (VkDeviceSize)nv * 16);
	cmdSubmitWait();
	if (getenv("CTR_RTX_DUMP"))
	{
		// diagnostics: per vertex pos, linear colour, tint, bake (sun, sky, bounce); then triangles
		FILE *df = fopen(getenv("CTR_RTX_DUMP"), "wb");
		if (df)
		{
			fwrite(&nv, 4, 1, df);
			fwrite(&nt, 4, 1, df);
			for (uint32_t v = 0; v < nv; v++)
			{
				float rec[17];
				memcpy(rec, verts[v].pos, 12);
				vertLinColor(&verts[v], &rec[3]);
				memcpy(&rec[6], &tint[v * 4], 16);
				memcpy(&rec[10], &bake[v * BAKE_STRIDE], 28);
				fwrite(rec, 4, 17, df);
			}
			fwrite(tris, sizeof(*tris), nt, df);
			fwrite(L->sunDir, 4, 3, df);
			fwrite(L->sunColor, 4, 3, df);
			fclose(df);
		}
	}

	// drop the lights the art doesn't support
	uint32_t kept = 0, before = R.numLights;
	for (uint32_t l = 0; l < R.numLights; l++)
	{
		if ((l < R.numFileLights) || (luma3(&lightData[l * 12 + 4]) >= 0.01f))
		{
			memmove(&lightData[kept * 12], &lightData[l * 12], 48);
			kept++;
		}
	}
	R.numLights = kept;
	logf_("art fit (%u vertices, shadows included): error %.3f -> %.3f, sun x%.2f, sky x%.2f, lights %u -> %u, %u glowing vertices", nfit,
	      nfit ? err0 / nfit : 0.0, nfit ? err1 / nfit : 0.0, scale[0], scale[1], before, kept, glowing);
	free(scale);
	free(num);
	free(den);
	free(lightLum);
	free(fitV);
}

// once per level, after the descriptor sets exist: lights from the art
static void levelPrep(void)
{
	uint32_t nv = R.hdr->numLevelVerts, nt = R.hdr->numLevelTris;
	const struct RtxLevelVert *verts = (const void *)(R.shm + RTX_OFF_LEVEL_VERTS);
	struct RtxLevelTri *tris = malloc(sizeof(*tris) * (nt ? nt : 1));
	memcpy(tris, R.shm + RTX_OFF_LEVEL_TRIS, sizeof(*tris) * nt);
	FrameData *F = R.frame.map;
	memset(F, 0, sizeof(*F));
	F->skyHorizon[3] = 2.0f; // ray epsilon
	F->extra2[0] = R.cornerBase;
	F->extra2[1] = CACHE_ENTRIES;

	// textures, for alpha-keyed shadow rays
	memcpy(R.texSlotsStage.map, R.shm + RTX_OFF_TEX_SLOTS, R.hdr->numTexSlots * sizeof(struct RtxTexSlot));
	memcpy(R.vramStage.map, R.shm + RTX_OFF_VRAM, RTX_VRAM_W * RTX_VRAM_H * 2);
	cmdBegin();
	copyBuf(&R.texSlotsStage, &R.texSlots, R.hdr->numTexSlots * sizeof(struct RtxTexSlot));
	copyBuf(&R.vramStage, &R.vram, RTX_VRAM_W * RTX_VRAM_H * 2);
	cmdSubmitWait();

	// what the lighting covers: visible textured surfaces, not water
	uint8_t *used = calloc(nv ? nv : 1, 1);
	for (uint32_t i = 0; i < nt; i++)
	{
		if ((tris[i].flags & RTX_TRI_TEXTURED) && !(tris[i].flags & RTX_TRI_WATER))
		{
			for (int c = 0; c < 3; c++)
			{
				used[tris[i].v[c]] = 1;
			}
		}
	}

	// sun and sky (outdoors), or the ambient (indoors / no sun found)
	struct RtxLighting *L = &R.light;
	float baseline[3] = {0, 0, 0};
	int outdoor = !L->indoor;
	double tp0 = nowMs();
	int sun = outdoor && (R.sunFit ? fitSun(verts, used, nv, baseline) : 1);
	double tpSun = nowMs();
	if (sun && !R.sunFit)
	{
		for (int c = 0; c < 3; c++)
		{
			baseline[c] = L->sunColor[c] + 0.5f * (L->skyZenith[c] + L->skyHorizon[c]); // the lights file's sun
		}
	}
	if (!sun)
	{
		// the darker half of the vertex colours: the light everywhere no lamp reaches
		float *lum = malloc(sizeof(float) * (nv ? nv : 1));
		int nu = 0;
		for (uint32_t i = 0; i < nv; i++)
		{
			float c[3];
			vertLinColor(&verts[i], c);
			if (used[i])
			{
				lum[nu++] = luma3(c);
			}
		}
		qsort(lum, nu, sizeof(float), cmpFloat);
		float median = nu ? lum[nu / 2] : 0.0f;
		free(lum);
		double sum[3] = {0, 0, 0};
		int count = 0;
		for (uint32_t i = 0; i < nv; i++)
		{
			float c[3];
			vertLinColor(&verts[i], c);
			if (used[i] && (luma3(c) <= median))
			{
				sum[0] += c[0];
				sum[1] += c[1];
				sum[2] += c[2];
				count++;
			}
		}
		for (int c = 0; c < 3; c++)
		{
			baseline[c] = count ? (float)(sum[c] / count) : 0.1f;
			if (!R.ambientSet)
			{
				L->ambient[c] = baseline[c];
			}
			L->skyZenith[c] = L->skyHorizon[c] = baseline[c];
			L->ground[c] = baseline[c] * 0.5f;
		}
		if (outdoor)
		{
			// evenly lit art (snow, night): no direction stands out, but outdoors
			// there still is a sun for shadows. From the brightest part of the
			// skybox, 45 degrees up; fitArt decides how strong it is
			int bestCol = 0;
			float bestLum = -1.0f;
			for (int c = 0; R.hdr->skyValid && (c < 8); c++)
			{
				float lum = luma3(R.hdr->sky[1][c]) + luma3(R.hdr->sky[2][c]);
				if (lum > bestLum)
				{
					bestLum = lum;
					bestCol = c;
				}
			}
			float az = ((bestCol + 0.5f) / 8.0f - 0.5f) * 6.2831853f, el = 0.785398f;
			L->sunDir[0] = cosf(el) * cosf(az);
			L->sunDir[1] = sinf(el);
			L->sunDir[2] = cosf(el) * sinf(az);
			for (int c = 0; c < 3; c++)
			{
				L->sunColor[c] = baseline[c] * 0.8f;
				L->skyZenith[c] = L->skyHorizon[c] = baseline[c] * 0.9f;
				L->ground[c] = baseline[c] * 0.35f;
			}
			sun = 1;
			logf_("outdoor without a clear sun in the art: sun from the sky (%.2f %.2f %.2f), sky %.3f %.3f %.3f", L->sunDir[0], L->sunDir[1], L->sunDir[2],
			      baseline[0], baseline[1], baseline[2]);
		}
		else
		{
			logf_("ambient from the vertex colours: %.3f %.3f %.3f", L->ambient[0], L->ambient[1], L->ambient[2]);
		}
	}

	// the sky the level shows (skybox colours), as bright as the fitted sky light
	R.skyValid = 0;
	if (sun && R.hdr->skyValid)
	{
		double avg = 0.0;
		for (int r = 0; r < 4; r++)
		{
			for (int c = 0; c < 8; c++)
			{
				float lin[3];
				for (int k = 0; k < 3; k++)
				{
					lin[k] = srgbLin(R.hdr->sky[r][c][k]);
				}
				avg += luma3(lin) / 32.0;
			}
		}
		float want = luma3(L->skyHorizon) / 0.9f;
		float scale = (avg > 1e-4) ? (float)(want / avg) : 0.0f;
		for (int r = 0; r < 8; r++)
		{
			for (int c = 0; c < 8; c++)
			{
				for (int k = 0; k < 3; k++)
				{
					R.sky[r * 8 + c][k] = srgbLin(R.hdr->sky[r][c][k]) * scale * (r >= 4 ? 0.5f : 1.0f); // below the horizon: mostly the level itself
				}
				R.sky[r * 8 + c][3] = 0.0f;
			}
		}
		// a near-black skybox (arenas, night) would only turn into noise when scaled up
		R.skyValid = (scale > 0.0f) && (scale <= 8.0f);
		logf_(R.skyValid ? "sky from the level's skybox colours (x%.2f)" : "skybox too dark to light with (x%.2f): fitted sky colour", scale);
	}

	// static lights: lights file, glowing surfaces, lamp pools
	float *lightData = R.lightsStage.map;
	R.numLights = 0;
	for (uint32_t i = 0; (i < L->numLights) && (R.numLights < RTX_MAX_LIGHTS); i++)
	{
		const struct RtxPointLight *pl = &L->lights[i];
		putLight(&lightData[R.numLights++ * 12], pl->pos, pl->height, pl->color, pl->falloff, pl->size, NULL);
	}
	R.numFileLights = R.numLights;
	double tpGlow0 = nowMs();
	addGlowLights(verts, tris, nt, lightData);
	markIce(verts, tris, nt);
	uint32_t firstHot = R.numLights;
	if (R.hotspots)
	{
		double tpHot0 = nowMs();
		addHotspots(verts, used, nv, baseline, lightData);
		if (getenv("CTR_RTX_DEBUG"))
		{
			logf_("  (prep: lamp pools %.0f ms)", nowMs() - tpHot0);
		}
	}
	uint32_t numHot = R.numLights - firstHot;

	// emissive flags -> level triangles; lights -> lifted off their surfaces
	Buf triStage = bufCreate(sizeof(*tris) * (nt ? nt : 1), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	memcpy(triStage.map, tris, sizeof(*tris) * nt);
	F->counts[0] = R.numLights;
	cmdBegin();
	copyBuf(&triStage, &R.levelTris, sizeof(*tris) * nt);
	copyBuf(&R.lightsStage, &R.lights, (VkDeviceSize)R.numLights * 48);
	buildLevelOnlyTlas();
	barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
	dispatchOnce(R.pipeLights, (R.numLights + 63) / 64, 0);
	barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
	copyBuf(&R.lights, &R.lightsReadback, (VkDeviceSize)R.numLights * 48);
	cmdSubmitWait();
	bufDestroy(&triStage);
	memcpy(lightData, R.lightsReadback.map, (size_t)R.numLights * 48);

	// lamp pool strengths, then the grid that finds lights near a point
	if (R.hotspots)
	{
		double tpFit0 = nowMs();
		fitHotspots(verts, nv, lightData, firstHot, numHot);
		if (getenv("CTR_RTX_DEBUG"))
		{
			logf_("  (prep: lamp pool fit %.0f ms)", nowMs() - tpFit0);
		}
	}
	double tpGrid0 = nowMs();
	buildLightGrid(verts, nv, lightData);
	double tpArt0 = nowMs();
	fitArt(verts, tris, nv, nt, used, lightData);
	double tpArt1 = nowMs();
	buildLightGrid(verts, nv, lightData);
	if (getenv("CTR_RTX_DEBUG"))
	{
		logf_("  (prep: sun fit %.0f ms, before glows %.0f ms, light grid %.0f ms, art fit %.0f ms)", tpSun - tp0, tpGlow0 - tpSun, tpArt0 - tpGrid0, tpArt1 - tpArt0);
	}
	cmdBegin();
	copyBuf(&R.lightsStage, &R.lights, (VkDeviceSize)R.numLights * 48);
	cmdSubmitWait();

	if (getenv("CTR_RTX_DEBUG"))
	{
		for (uint32_t l = 0; l < R.numLights && l < 2000; l += (R.numLights / 24) + 1)
		{
			const float *d = &lightData[l * 12];
			logf_("  light %u (%s): %.0f %.0f %.0f colour %.2f %.2f %.2f falloff %.0f", l,
			      l < R.numFileLights ? "file" : (l < firstHot ? "glow" : "pool"), d[0], d[1], d[2], d[4], d[5], d[6], d[7]);
		}
	}
	logf_("lights: %u from file, %u glowing surfaces, %u lamp pools", R.numFileLights, R.numGlowLights, numHot);
	free(used);
	free(tris);
	R.levelPrep = 0;
}

// ---- per-pixel buffers ------------------------------------------------------------------

// traced size inW x inH, output (TAA) size w x h
static void resize(uint32_t inW, uint32_t inH, uint32_t w, uint32_t h)
{
	vkDeviceWaitIdle(R.dev);
	Buf *bufs[] = {&R.albedo, &R.geo[0], &R.geo[1], &R.hist[0], &R.hist[1], &R.histDyn[0], &R.histDyn[1], &R.moments[0], &R.moments[1], &R.reservoir[0], &R.reservoir[1], &R.combined, &R.filterA, &R.filterB, &R.layer, &R.disp, &R.spec[0], &R.spec[1], &R.fog[0], &R.fog[1]};
	for (unsigned i = 0; i < sizeof(bufs) / sizeof(*bufs); i++)
	{
		bufDestroy(bufs[i]);
		*bufs[i] = bufCreate((VkDeviceSize)inW * inH * 16, USAGE_STORAGE, DEVICE_LOCAL);
	}
	for (int i = 0; i < 2; i++)
	{
		bufDestroy(&R.taaHist[i]);
		R.taaHist[i] = bufCreate((VkDeviceSize)w * h * 16, USAGE_STORAGE, DEVICE_LOCAL);
	}
	bufDestroy(&R.out);
	bufDestroy(&R.readback);
	bufDestroy(&R.depth);
	bufDestroy(&R.depthReadback);
	R.out = bufCreate((VkDeviceSize)w * h * 4, USAGE_STORAGE, DEVICE_LOCAL);
	R.readback = bufCreate((VkDeviceSize)w * h * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, HOST_MEM | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	R.depth = bufCreate((VkDeviceSize)w * h * 4, USAGE_STORAGE, DEVICE_LOCAL);
	R.depthReadback = bufCreate((VkDeviceSize)w * h * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, HOST_MEM | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	// geo/hist start invalid
	cmdBegin();
	for (int i = 0; i < 2; i++)
	{
		vkCmdFillBuffer(R.cmd, R.geo[i].buf, 0, VK_WHOLE_SIZE, 0);
		vkCmdFillBuffer(R.cmd, R.hist[i].buf, 0, VK_WHOLE_SIZE, 0);
		vkCmdFillBuffer(R.cmd, R.histDyn[i].buf, 0, VK_WHOLE_SIZE, 0);
		vkCmdFillBuffer(R.cmd, R.moments[i].buf, 0, VK_WHOLE_SIZE, 0);
		vkCmdFillBuffer(R.cmd, R.reservoir[i].buf, 0, VK_WHOLE_SIZE, 0);
		vkCmdFillBuffer(R.cmd, R.taaHist[i].buf, 0, VK_WHOLE_SIZE, 0);
		vkCmdFillBuffer(R.cmd, R.spec[i].buf, 0, VK_WHOLE_SIZE, 0);
		vkCmdFillBuffer(R.cmd, R.fog[i].buf, 0, VK_WHOLE_SIZE, 0);
	}
	cmdSubmitWait();
	R.outW = w;
	R.outH = h;
	R.inW = inW;
	R.inH = inH;
	R.hdr->inW = inW;
	R.hdr->inH = inH;
	R.havePrev = 0;
	logf_("traced %ux%u, output %ux%u", inW, inH, w, h);
}

static void writeSets(void)
{
	for (int par = 0; par < 2; par++)
	{
		Buf *geoCur = &R.geo[par], *geoPrev = &R.geo[par ^ 1], *histCur = &R.hist[par], *histPrev = &R.hist[par ^ 1];
		for (int pass = 0; pass < PASS_COUNT; pass++)
		{
			// a-trous k (1-based) ping-pongs combined -> A -> B -> A ...; final reads A
			Buf *src = &R.combined, *dst = &R.filterA;
			if ((pass > PASS_ATROUS1) && (pass < PASS_FINAL))
			{
				int k = pass - PASS_ATROUS1 + 1;
				src = (k % 2 == 0) ? &R.filterA : &R.filterB;
				dst = (k % 2 == 1) ? &R.filterA : &R.filterB;
			}
			else if (pass == PASS_FINAL)
			{
				src = &R.filterA;
				dst = &R.filterB; // final: what glows (linear), for the bloom
			}
			else if (pass == PASS_BLOOM_H)
			{
				src = &R.filterB;
				dst = &R.filterA;
			}
			else if (pass == PASS_BLOOM_V)
			{
				src = &R.filterA;
				dst = &R.filterB;
			}
			Buf *b[B_COUNT] = {NULL,    &R.frame, &R.levelVerts, &R.levelTris, &R.texSlots, &R.dynTris,        &R.vram,        &R.lights, &R.albedo, geoCur,
			                   geoPrev, histPrev, histCur,       src,          dst,         &R.out,            &R.lum,         &R.levelNormals, &R.dynNormals, &R.layer, &R.depth,
			                   &R.cacheKeys, &R.cacheAccum, &R.cacheVal, &R.lightGrid, &R.fit, &R.capTris,
			                   &R.histDyn[par ^ 1], &R.histDyn[par], &R.combined, &R.bake, &R.levelTint, &R.moments[par ^ 1], &R.moments[par], &R.reservoir[par ^ 1], &R.reservoir[par],
			                   &R.disp, &R.taaHist[par ^ 1], &R.taaHist[par], &R.blueNoise, &R.spec[par ^ 1], &R.spec[par],
			                   &R.fog[par ^ 1], &R.fog[par]};
			VkDescriptorBufferInfo infos[B_COUNT];
			VkWriteDescriptorSet w[B_COUNT];
			VkWriteDescriptorSetAccelerationStructureKHR asw = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR, .accelerationStructureCount = 1,
			                                                     .pAccelerationStructures = &R.tlas.as};
			for (int i = 0; i < B_COUNT; i++)
			{
				w[i] = (VkWriteDescriptorSet){VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = R.sets[par][pass], .dstBinding = (uint32_t)i, .descriptorCount = 1};
				if (i == B_TLAS)
				{
					w[i].pNext = &asw;
					w[i].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				}
				else
				{
					infos[i] = (VkDescriptorBufferInfo){b[i]->buf, 0, VK_WHOLE_SIZE};
					w[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					w[i].pBufferInfo = &infos[i];
				}
			}
			vkUpdateDescriptorSets(R.dev, B_COUNT, w, 0, NULL);
		}
	}
}

// blue noise: a 64x64 tile of ranks (0..1) by void and cluster (Ulichney 1993):
// every value once, neighbours far apart in value. The first samples of each
// pixel (trace.comp) take it instead of white noise: the error is spread finely
// over the image, where the denoiser and TAA take it out
#define BN_SIZE 64
static void blueNoiseTile(float *out)
{
	const int N = BN_SIZE * BN_SIZE;
	static float lut[BN_SIZE][BN_SIZE]; // gaussian (sigma 1.5) by wrapped distance
	for (int dy = 0; dy < BN_SIZE; dy++)
	{
		for (int dx = 0; dx < BN_SIZE; dx++)
		{
			int ax = dx < BN_SIZE - dx ? dx : BN_SIZE - dx, ay = dy < BN_SIZE - dy ? dy : BN_SIZE - dy;
			lut[dy][dx] = expf(-(float)(ax * ax + ay * ay) / (2.0f * 1.5f * 1.5f));
		}
	}
	uint8_t *bits = calloc(N, 1), *proto = calloc(N, 1);
	float *energy = calloc(N, sizeof(float));
	int *rank = malloc(N * sizeof(int));
	// (energy: how crowded each pixel's neighbourhood is with ones)
#define BN_TOGGLE(i, sgn)                                                                       \
	do                                                                                          \
	{                                                                                           \
		int xi_ = (i) % BN_SIZE, yi_ = (i) / BN_SIZE;                                           \
		for (int j_ = 0; j_ < N; j_++)                                                          \
		{                                                                                       \
			energy[j_] += (sgn) * lut[(j_ / BN_SIZE - yi_) & (BN_SIZE - 1)][(j_ % BN_SIZE - xi_) & (BN_SIZE - 1)]; \
		}                                                                                       \
	} while (0)
	uint32_t seed = 0x9e3779b9u;
	int ones = 0;
	while (ones < N / 10)
	{
		seed ^= seed << 13;
		seed ^= seed >> 17;
		seed ^= seed << 5;
		int i = (int)(seed % (uint32_t)N);
		if (!bits[i])
		{
			bits[i] = 1;
			BN_TOGGLE(i, 1.0f);
			ones++;
		}
	}
	// initial pattern: move the tightest cluster's one into the largest void until stable
	for (int iter = 0; iter < 4 * N; iter++)
	{
		int tight = -1, loose = -1;
		for (int j = 0; j < N; j++)
		{
			tight = (bits[j] && ((tight < 0) || (energy[j] > energy[tight]))) ? j : tight;
		}
		bits[tight] = 0;
		BN_TOGGLE(tight, -1.0f);
		for (int j = 0; j < N; j++)
		{
			loose = (!bits[j] && ((loose < 0) || (energy[j] < energy[loose]))) ? j : loose;
		}
		bits[loose] = 1;
		BN_TOGGLE(loose, 1.0f);
		if (loose == tight)
		{
			break;
		}
	}
	memcpy(proto, bits, N);
	float *protoEnergy = malloc(N * sizeof(float));
	memcpy(protoEnergy, energy, N * sizeof(float));
	// ranks below the pattern's: take away the tightest clusters
	for (int r = ones - 1; r >= 0; r--)
	{
		int tight = -1;
		for (int j = 0; j < N; j++)
		{
			tight = (bits[j] && ((tight < 0) || (energy[j] > energy[tight]))) ? j : tight;
		}
		bits[tight] = 0;
		BN_TOGGLE(tight, -1.0f);
		rank[tight] = r;
	}
	// ranks above: fill the largest voids
	memcpy(bits, proto, N);
	memcpy(energy, protoEnergy, N * sizeof(float));
	for (int r = ones; r < N; r++)
	{
		int loose = -1;
		for (int j = 0; j < N; j++)
		{
			loose = (!bits[j] && ((loose < 0) || (energy[j] < energy[loose]))) ? j : loose;
		}
		bits[loose] = 1;
		BN_TOGGLE(loose, 1.0f);
		rank[loose] = r;
	}
#undef BN_TOGGLE
	for (int i = 0; i < N; i++)
	{
		out[i] = ((float)rank[i] + 0.5f) / (float)N;
	}
	free(bits);
	free(proto);
	free(energy);
	free(protoEnergy);
	free(rank);
}

static void initFrameBuffers(void)
{
	{
		static float bn[BN_SIZE * BN_SIZE];
		double t0 = nowMs();
		blueNoiseTile(bn);
		R.blueNoise = uploadBuf(bn, sizeof(bn), 0);
		logf_("blue noise %dx%d in %.0f ms", BN_SIZE, BN_SIZE, nowMs() - t0);
	}
	R.frame = bufCreate(sizeof(FrameData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, HOST_MEM);
	R.texSlots = bufCreate(RTX_MAX_TEX_SLOTS * sizeof(struct RtxTexSlot), USAGE_STORAGE, DEVICE_LOCAL);
	R.texSlotsStage = bufCreate(RTX_MAX_TEX_SLOTS * sizeof(struct RtxTexSlot), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.dynTris = bufCreate(RTX_MAX_DYN_TRIS * sizeof(struct RtxDynTri), USAGE_STORAGE, DEVICE_LOCAL);
	R.dynTrisStage = bufCreate(RTX_MAX_DYN_TRIS * sizeof(struct RtxDynTri), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.dynPos = bufCreate(RTX_MAX_DYN_TRIS * 36, USAGE_STORAGE | USAGE_AS_INPUT, DEVICE_LOCAL);
	R.dynPosStage = bufCreate(RTX_MAX_DYN_TRIS * 36, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.dynNormals = bufCreate(RTX_MAX_DYN_TRIS * 48, USAGE_STORAGE, DEVICE_LOCAL);
	R.dynNormalsStage = bufCreate(RTX_MAX_DYN_TRIS * 48, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.lights = bufCreate((RTX_MAX_LIGHTS + RTX_MAX_DYN_LIGHTS) * 48, USAGE_STORAGE, DEVICE_LOCAL);
	R.lightsStage = bufCreate((RTX_MAX_LIGHTS + RTX_MAX_DYN_LIGHTS) * 48, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.lightsReadback = bufCreate(RTX_MAX_LIGHTS * 48, VK_BUFFER_USAGE_TRANSFER_DST_BIT, HOST_MEM | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	R.cacheKeys = bufCreate((VkDeviceSize)CACHE_ENTRIES * 8, USAGE_STORAGE, DEVICE_LOCAL);
	R.cacheAccum = bufCreate((VkDeviceSize)CACHE_ENTRIES * 16, USAGE_STORAGE, DEVICE_LOCAL);
	R.cacheVal = bufCreate((VkDeviceSize)CACHE_ENTRIES * 16, USAGE_STORAGE, DEVICE_LOCAL);
	R.lightGrid = bufCreate(LIGHT_GRID_WORDS * 4, USAGE_STORAGE, HOST_MEM);
	R.fit = bufCreate(FIT_VEC4S * 16, USAGE_STORAGE, HOST_MEM | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	R.bake = bufCreate((VkDeviceSize)RTX_MAX_LEVEL_VERTS * (8 + 2 * 48) * 4, USAGE_STORAGE, HOST_MEM | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	R.levelTint = bufCreate(RTX_MAX_LEVEL_VERTS * 16, USAGE_STORAGE, DEVICE_LOCAL);
	R.levelTintStage = bufCreate(RTX_MAX_LEVEL_VERTS * 16, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.vram = bufCreate(RTX_VRAM_W * RTX_VRAM_H * 2, USAGE_STORAGE, DEVICE_LOCAL);
	R.vramStage = bufCreate(RTX_VRAM_W * RTX_VRAM_H * 2, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.tlasInstances = bufCreate(4 * sizeof(VkAccelerationStructureInstanceKHR), USAGE_AS_INPUT, HOST_MEM);
	R.capTris = bufCreate(RTX_MAX_LEVEL_CAP * sizeof(struct RtxDynTri), USAGE_STORAGE, DEVICE_LOCAL);
	R.capTrisStage = bufCreate(RTX_MAX_LEVEL_CAP * sizeof(struct RtxDynTri), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.capPos = bufCreate(RTX_MAX_LEVEL_CAP * 36, USAGE_STORAGE | USAGE_AS_INPUT, DEVICE_LOCAL);
	R.capPosStage = bufCreate(RTX_MAX_LEVEL_CAP * 36, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.lum = bufCreate(512, USAGE_STORAGE, HOST_MEM); // exposure sums, texture diagnostics

	// TLAS sized for 4 instances (level, models, texture layer, the level forced opaque), built every frame
	VkAccelerationStructureGeometryKHR g = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR};
	g.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	g.geometry.instances.data.deviceAddress = R.tlasInstances.addr;
	VkAccelerationStructureBuildGeometryInfoKHR bi = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	                                                   .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	                                                   .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	                                                   .geometryCount = 1,
	                                                   .pGeometries = &g};
	uint32_t maxInst = 4;
	VkAccelerationStructureBuildSizesInfoKHR sizes = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	vkGetAccelerationStructureBuildSizesKHR(R.dev, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &bi, &maxInst, &sizes);
	accelCreate(&R.tlas, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, sizes.accelerationStructureSize);

	// scratch for the biggest per-frame build up front: growing it (scratchAddr)
	// while a frame's builds are recorded would free memory they use
	VkAccelerationStructureGeometryKHR tg = triGeometry(R.capPos.addr, 12, RTX_MAX_LEVEL_CAP * 3 - 1, 0);
	VkAccelerationStructureBuildGeometryInfoKHR tbi = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	                                                    .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	                                                    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR,
	                                                    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	                                                    .geometryCount = 1,
	                                                    .pGeometries = &tg};
	uint32_t maxTris = RTX_MAX_LEVEL_CAP > RTX_MAX_DYN_TRIS ? RTX_MAX_LEVEL_CAP : RTX_MAX_DYN_TRIS;
	VkAccelerationStructureBuildSizesInfoKHR tsizes = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	vkGetAccelerationStructureBuildSizesKHR(R.dev, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &tbi, &maxTris, &tsizes);
	scratchAddr(tsizes.buildScratchSize > sizes.buildScratchSize ? tsizes.buildScratchSize : sizes.buildScratchSize);
}

// ---- frame ------------------------------------------------------------------------------

static void invert3(const float *m, float *out)
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

// radical inverse of i in base b (0..1)
static float halton(uint32_t i, uint32_t b)
{
	float f = 1.0f, r = 0.0f;
	while (i > 0u)
	{
		f /= (float)b;
		r += f * (float)(i % b);
		i /= b;
	}
	return r;
}

static void renderFrame(void)
{
	double tStart = nowMs();
	// this frame's header as the game wrote it (pipelined it writes the next
	// frame's once frameTaken says the input is copied)
	static struct RtxHeader Hsnap;
	memcpy(&Hsnap, (const void *)R.hdr, sizeof(Hsnap));
	struct RtxHeader *H = &Hsnap;
	uint32_t nd = H->numDynTris;
	if (nd > RTX_MAX_DYN_TRIS)
	{
		nd = RTX_MAX_DYN_TRIS;
	}

	// ---- frame constants
	FrameData *F = R.frame.map;
	memset(F, 0, sizeof(*F));
	float rInv[9];
	invert3(H->camR, rInv);
	float camPos[3];
	for (int r = 0; r < 3; r++)
	{
		camPos[r] = -(rInv[r * 3] * H->camT[0] + rInv[r * 3 + 1] * H->camT[1] + rInv[r * 3 + 2] * H->camT[2]);
		for (int c = 0; c < 3; c++)
		{
			F->rInv[r][c] = rInv[r * 3 + c];
			F->camR[r][c] = H->camR[r * 3 + c];
		}
		F->camPos[r] = camPos[r];
		F->camT[r] = H->camT[r];
	}
	int reset = !R.havePrev || (H->frame != R.lastGameFrame + 1) || H->resetHistory || (H->lightMode != R.lastLightMode);
	R.lastLightMode = H->lightMode;
	if (R.havePrev)
	{
		float dx = camPos[0] - R.prevCamPos[0], dy = camPos[1] - R.prevCamPos[1], dz = camPos[2] - R.prevCamPos[2];
		if (dx * dx + dy * dy + dz * dz > 3000.0f * 3000.0f)
		{
			reset = 1; // camera cut
		}
	}
	for (int r = 0; r < 3; r++)
	{
		for (int c = 0; c < 3; c++)
		{
			F->prevR[r][c] = R.havePrev ? R.prevR[r * 3 + c] : H->camR[r * 3 + c];
		}
		F->prevT[r] = R.havePrev ? R.prevT[r] : H->camT[r];
	}
	F->proj[0] = H->projH;
	F->proj[1] = H->vpX;
	F->proj[2] = H->vpY;
	F->proj[3] = H->vpW;
	F->proj2[0] = H->vpH;
	F->proj2[1] = H->dispW;
	F->proj2[2] = H->dispH;
	F->proj2[3] = R.havePrev ? R.prevH : H->projH;
	F->size[0] = R.inW;
	F->size[1] = R.inH;
	F->outSize[0] = R.outW;
	F->outSize[1] = R.outH;
	F->outSize[2] = H->settings.taa ? 1u : 0u;
	// Halton (2, 3), 16 frames: the samples cover each pixel evenly
	F->jitter[0] = halton(1u + R.frameIndex % 16u, 2u);
	F->jitter[1] = halton(1u + R.frameIndex % 16u, 3u);
	F->feat[0] = H->settings.texFilter;
	F->feat[1] = H->settings.specular;
	F->feat[2] = H->settings.fog;
	F->size[2] = R.frameIndex++;
	F->size[3] = H->debugView;

	struct RtxLighting *L = &R.light;
	float sunLen = sqrtf(L->sunDir[0] * L->sunDir[0] + L->sunDir[1] * L->sunDir[1] + L->sunDir[2] * L->sunDir[2]);
	for (int i = 0; i < 3; i++)
	{
		F->sunDir[i] = L->sunDir[i] / (sunLen > 0 ? sunLen : 1);
		F->sunColor[i] = L->indoor ? 0.0f : L->sunColor[i];
		F->skyZenith[i] = L->skyZenith[i];
		F->skyHorizon[i] = L->skyHorizon[i];
		F->groundColor[i] = L->ground[i];
		F->ambient[i] = L->ambient[i];
	}
	F->sunDir[3] = L->sunCone;
	// exposure: light units are the PS1's (linear vertex colour), so 1 is about
	// right; trace.comp measures how bright the PS1 drew the frame vs the traced
	// one and it follows that
	float exposure = (L->exposure > 0.0f) ? L->exposure : ((R.autoExposure > 0.0f) ? R.autoExposure : 1.0f);
	int hybrid = (H->lightMode == 0);
	F->sunColor[3] = hybrid ? 1.0f : exposure; // hybrid: the PS1 colours as they are
	F->skyZenith[3] = 0.0f;
	F->skyHorizon[3] = 2.0f;   // ray epsilon (level units)
	F->groundColor[3] = 64.0f; // max accumulated frames
	F->ambient[3] = L->indoor ? 1.0f : 0.0f;
	F->counts[0] = R.numLights;
	F->counts[1] = H->numLevelTris;
	F->counts[2] = nd;
	F->counts[3] = reset ? 1u : 0u;
	F->params[0] = 1500.0f; // indoor AO radius
	F->params[1] = (float)R.frameIndex / 30.0f; // seconds, water ripples
	F->params[2] = R.levelFront;
	F->params[3] = getenv("CTR_RTX_EXPERIMENT") ? (float)atof(getenv("CTR_RTX_EXPERIMENT")) : 0.0f; // 1: every ray opaque (wrong image: a timing bound)
	uint32_t ndl = H->numDynLights > RTX_MAX_DYN_LIGHTS ? RTX_MAX_DYN_LIGHTS : H->numDynLights;
	F->extra[0] = ndl;
	F->extra[2] = H->lightMode;
	F->extra[3] = R.spp;
	F->params2[1] = 24.0f; // radiance cache cell size near the camera
	// haze: density at the camera's height (outdoors ~90% clear over 10000 units),
	// e-folding height; CTR_RTX_FOG_DENSITY scales it
	static float fogScale = -1.0f;
	if (fogScale < 0.0f)
	{
		fogScale = getenv("CTR_RTX_FOG_DENSITY") ? (float)atof(getenv("CTR_RTX_FOG_DENSITY")) : 1.0f;
	}
	F->params2[0] = (L->indoor ? 0.6e-5f : 1.0e-5f) * fogScale;
	F->params2[2] = 2000.0f;
	F->params2[3] = H->texNearDepth;
	F->extra2[0] = R.cornerBase;
	F->extra2[1] = CACHE_ENTRIES;
	F->extra2[2] = 0;
	for (int i = 0; i < 4; i++)
	{
		F->gridMin[i] = R.gridMin[i];
	}
	for (int i = 0; i < 3; i++)
	{
		F->gridDim[i] = R.gridDim[i];
	}
	F->gridDim[3] = R.skyValid;
	memcpy(F->sky, R.sky, sizeof(F->sky));

	// ---- uploads
	memcpy(R.texSlotsStage.map, R.shm + RTX_OFF_TEX_SLOTS, H->numTexSlots * sizeof(struct RtxTexSlot));
	memcpy(R.vramStage.map, R.shm + RTX_OFF_VRAM, RTX_VRAM_W * RTX_VRAM_H * 2);
	static struct RtxDynTri dyn[RTX_MAX_DYN_TRIS];
	memcpy(dyn, R.shm + RTX_OFF_DYN_TRIS, nd * sizeof(struct RtxDynTri));
	if (!L->indoor)
	{
		smokeFaceSun(dyn, nd, F->sunDir);
	}
	memcpy(R.dynTrisStage.map, dyn, nd * sizeof(struct RtxDynTri));
	float *dp = R.dynPosStage.map;
	for (uint32_t i = 0; i < nd; i++)
	{
		memcpy(&dp[i * 9], dyn[i].p, 36);
	}
	dynSmoothNormals(dyn, nd, R.dynNormalsStage.map, camPos);
	uint32_t nc = H->numLevelCap > RTX_MAX_LEVEL_CAP ? RTX_MAX_LEVEL_CAP : H->numLevelCap;
	const struct RtxDynTri *cap = (const void *)(R.shm + RTX_OFF_LEVEL_CAP);
	memcpy(R.capTrisStage.map, cap, nc * sizeof(struct RtxDynTri));
	float *cp = R.capPosStage.map;
	for (uint32_t i = 0; i < nc; i++)
	{
		memcpy(&cp[i * 9], cap[i].p, 36);
	}
	float *ls = (float *)R.lightsStage.map + R.numLights * 12; // after the static lights
	for (uint32_t i = 0; i < ndl; i++)
	{
		const struct RtxPointLight *l = &H->dynLights[i];
		float rec[12] = {l->pos[0], l->pos[1], l->pos[2], 0.0f, l->color[0], l->color[1], l->color[2], l->falloff, l->size, 0, 0, 0};
		memcpy(&ls[i * 12], rec, sizeof(rec));
	}

	VkAccelerationStructureInstanceKHR inst[4];
	memset(inst, 0, sizeof(inst));
	uint32_t numInst = 0;
	for (int k = 0; k < 3; k++)
	{
		if (((k == 1) && (nd == 0)) || ((k == 2) && (nc == 0)))
		{
			continue;
		}
		VkAccelerationStructureInstanceKHR *I = &inst[numInst++];
		I->transform.matrix[0][0] = I->transform.matrix[1][1] = I->transform.matrix[2][2] = 1.0f;
		I->instanceCustomIndex = (uint32_t)k;
		I->mask = 1u << k;
		I->accelerationStructureReference = (k == 0) ? R.levelBlas.addr : 0; // models / texture layer: after their BLAS builds
	}
	// the level again, forced opaque (mask 0x8): bounce rays take it, so the RT
	// hardware settles their level hits without the shaders' per-candidate tests
	inst[numInst] = inst[0];
	inst[numInst].mask = 0x8;
	inst[numInst].flags = VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
	numInst++;

	// the game's input is copied: pipelined, the game goes on with its next frame
	__sync_synchronize();
	R.hdr->frameTaken = H->frameReq;
	futexWake(&R.hdr->frameTaken);

	cmdBegin();
	if (R.haveTimestamps)
	{
		vkCmdResetQueryPool(R.cmd, R.timestamps, 0, TS_COUNT);
	}
	stamp(TS_START);
	copyBuf(&R.texSlotsStage, &R.texSlots, H->numTexSlots * sizeof(struct RtxTexSlot));
	copyBuf(&R.vramStage, &R.vram, RTX_VRAM_W * RTX_VRAM_H * 2);
	copyBuf(&R.dynTrisStage, &R.dynTris, nd * sizeof(struct RtxDynTri));
	copyBuf(&R.dynPosStage, &R.dynPos, nd * 36);
	copyBuf(&R.dynNormalsStage, &R.dynNormals, nd * 48);
	copyBuf(&R.capTrisStage, &R.capTris, nc * sizeof(struct RtxDynTri));
	copyBuf(&R.capPosStage, &R.capPos, nc * 36);
	vkCmdFillBuffer(R.cmd, R.lum.buf, 0, VK_WHOLE_SIZE, 0);
	if (ndl > 0)
	{
		VkBufferCopy lc = {(VkDeviceSize)R.numLights * 48, (VkDeviceSize)R.numLights * 48, (VkDeviceSize)ndl * 48};
		vkCmdCopyBuffer(R.cmd, R.lightsStage.buf, R.lights.buf, 1, &lc);
	}
	barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);

	for (uint32_t i = 0; i < numInst; i++)
	{
		uint32_t k = inst[i].instanceCustomIndex;
		if (k == 0)
		{
			continue;
		}
		Accel *blas = (k == 1) ? &R.dynBlas : &R.capBlas;
		uint32_t count = (k == 1) ? nd : nc;
		VkAccelerationStructureGeometryKHR g = triGeometry(((k == 1) ? R.dynPos : R.capPos).addr, 12, count * 3 - 1, 0);
		buildBlas(blas, &g, count, (k == 1) ? RTX_MAX_DYN_TRIS : RTX_MAX_LEVEL_CAP, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR);
		inst[i].accelerationStructureReference = blas->addr;
		// BLAS -> next build (shared scratch buffer) and the TLAS
		barrier(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
		        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
		        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
	}
	memcpy(R.tlasInstances.map, inst, sizeof(inst));

	{
		VkAccelerationStructureGeometryKHR g = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR};
		g.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		g.geometry.instances.data.deviceAddress = R.tlasInstances.addr;
		VkAccelerationStructureBuildGeometryInfoKHR bi = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		                                                   .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
		                                                   .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		                                                   .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		                                                   .dstAccelerationStructure = R.tlas.as,
		                                                   .geometryCount = 1,
		                                                   .pGeometries = &g};
		uint32_t maxInst = 4;
		VkAccelerationStructureBuildSizesInfoKHR sizes = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
		vkGetAccelerationStructureBuildSizesKHR(R.dev, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &bi, &maxInst, &sizes);
		bi.scratchData.deviceAddress = scratchAddr(sizes.buildScratchSize);
		VkAccelerationStructureBuildRangeInfoKHR range = {.primitiveCount = numInst};
		const VkAccelerationStructureBuildRangeInfoKHR *pr = &range;
		vkCmdBuildAccelerationStructuresKHR(R.cmd, 1, &bi, &pr);
	}
	barrier(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT);

	uint32_t gx = (R.inW + 7) / 8, gy = (R.inH + 7) / 8;
	VkDescriptorSet *sets = R.sets[R.parity];
	stamp(TS_BUILT);
	vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeTrace);
	vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &sets[PASS_TRACE], 0, NULL);
	vkCmdDispatch(R.cmd, gx, gy, 1);
	stamp(TS_TRACED);
	if (!hybrid)
	{
		// this frame's radiance cache writes -> cell values
		barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
		vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeCache);
		vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &sets[PASS_TRACE], 0, NULL);
		vkCmdDispatch(R.cmd, CACHE_ENTRIES / 256, 1, 1);
	}
	stamp(TS_CACHED);

	vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeAtrous);
	static const int steps[NUM_ATROUS] = {1, 2, 4, 8, 16};
	for (int i = 0; i < NUM_ATROUS; i++)
	{
		barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
		vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &sets[PASS_ATROUS1 + i], 0, NULL);
		vkCmdPushConstants(R.cmd, R.pipeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4, &steps[i]);
		vkCmdDispatch(R.cmd, gx, gy, 1);
	}
	stamp(TS_FILTERED);

	barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
	vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeFinal);
	vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &sets[PASS_FINAL], 0, NULL);
	vkCmdDispatch(R.cmd, gx, gy, 1);

	// bloom: what glows, blurred (horizontal, then vertical added onto the image)
	vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeBloom);
	for (int b = 0; b < 2; b++)
	{
		barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
		vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &sets[PASS_BLOOM_H + b], 0, NULL);
		vkCmdPushConstants(R.cmd, R.pipeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4, &b);
		vkCmdDispatch(R.cmd, gx, gy, 1);
	}

	// TAA / upscale onto the output size: image, depth
	barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
	vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeTaa);
	vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &sets[PASS_TAA], 0, NULL);
	vkCmdDispatch(R.cmd, (R.outW + 7) / 8, (R.outH + 7) / 8, 1);

	stamp(TS_FINISHED);
	barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
	copyBuf(&R.out, &R.readback, (VkDeviceSize)R.outW * R.outH * 4);
	copyBuf(&R.depth, &R.depthReadback, (VkDeviceSize)R.outW * R.outH * 4);
	stamp(TS_COPIED);
	double tSubmit = nowMs();
	cmdSubmitWait();
	double tDone = nowMs();

	uint32_t slot = H->frameReq & 1u;
	memcpy(R.shm + RTX_OFF_IMAGE + slot * RTX_SLOT_BYTES, R.readback.map, (size_t)R.outW * R.outH * 4);
	memcpy(R.shm + RTX_OFF_DEPTH + slot * RTX_SLOT_BYTES, R.depthReadback.map, (size_t)R.outW * R.outH * 4);

	// exposure that makes the traced frame as bright as the PS1 one (smoothed)
	const uint32_t *lum = R.lum.map;
	if (!hybrid && (lum[2] > 64) && (lum[1] > 0))
	{
		float target = fminf(fmaxf((float)lum[0] / (float)lum[1], 0.1f), 16.0f);
		R.autoExposure = (R.autoExposure > 0.0f) ? R.autoExposure + (target - R.autoExposure) * 0.05f : target;
		if (getenv("CTR_RTX_DEBUG") && (R.frameIndex % 300 == 0))
		{
			logf_("exposure %.2f (PS1 frame %.3f, traced %.3f), %u spp, %u static + %u frame lights, %u level polygons drawn", R.autoExposure,
			      lum[0] / 1024.0 / lum[2], lum[1] / 1024.0 / lum[2], R.spp, R.numLights, ndl, nc);

			logf_("texture layer: %u of %u level pixels; level mesh texels differ from the PS1's on %u; %u level pixels touched by models", lum[4], lum[3], lum[5], lum[7]);
			const struct RtxLevelTri *lt = (const void *)(R.shm + RTX_OFF_LEVEL_TRIS);
			const struct RtxTexSlot *ls = (const void *)(R.shm + RTX_OFF_TEX_SLOTS);
			const struct RtxDynTri *ct = R.capTrisStage.map;
			for (uint32_t k = 0; (k < lum[6]) && (k < 6); k++)
			{
				uint32_t sp = lum[8 + k * 2], cpi = lum[9 + k * 2];
				if ((sp >= H->numLevelTris) || (cpi >= nc))
				{
					continue;
				}
				const struct RtxLevelTri *t = &lt[sp];
				const struct RtxTexSlot *sl = (t->texSlot < H->numTexSlots) ? &ls[t->texSlot] : NULL;
				const struct RtxDynTri *c = &ct[cpi];
				logf_("  mesh tri %u slot %u corners %u%u%u flags %x tpage %04x clut %04x uv %u,%u %u,%u %u,%u | PS1 tpage %04x clut %04x uv %u,%u %u,%u %u,%u", sp,
				      t->texSlot, t->corner[0], t->corner[1], t->corner[2], t->flags, sl ? sl->tpage : 0, sl ? sl->clut : 0,
				      sl ? sl->uv[t->corner[0]][0] : 0, sl ? sl->uv[t->corner[0]][1] : 0, sl ? sl->uv[t->corner[1]][0] : 0,
				      sl ? sl->uv[t->corner[1]][1] : 0, sl ? sl->uv[t->corner[2]][0] : 0, sl ? sl->uv[t->corner[2]][1] : 0, c->tpage, c->clut,
				      c->uv[0][0], c->uv[0][1], c->uv[1][0], c->uv[1][1], c->uv[2][0], c->uv[2][1]);
			}
		}
	}

	// next frame's samples per pixel: the quality preset's, fewer if the trace
	// pass would overrun its budget
	static const uint32_t presetSpp[4] = {2, 4, 8, 16};
	uint32_t preset = presetSpp[H->settings.quality & 3u];
	R.budgetMs = (H->settings.budgetMs > 0.0f) ? H->settings.budgetMs : R.budgetMs;
	if (!R.haveTimestamps)
	{
		R.spp = preset;
		R.hdr->traceSpp = R.spp;
	}
	if (R.haveTimestamps)
	{
		uint64_t ts[TS_COUNT];
		if (vkGetQueryPoolResults(R.dev, R.timestamps, 0, TS_COUNT, sizeof(ts), ts, sizeof(uint64_t), VK_QUERY_RESULT_64_BIT) == VK_SUCCESS)
		{
			float ms = (float)(ts[TS_TRACED] - ts[TS_BUILT]) * R.timestampPeriodNs / 1e6f;
			float per = ms / (float)R.spp;
			R.statTraceMs += ms;
			R.statSppMs += per;
			// every pass's time: smoothed for the overlay, averaged for the log
			float pass[RTX_PASS_COUNT];
			for (int i = 0; i < TS_COPIED; i++)
			{
				pass[i] = (float)(ts[i + 1] - ts[i]) * R.timestampPeriodNs / 1e6f;
			}
			pass[RTX_PASS_CPU] = (float)((tSubmit - tStart) + (nowMs() - tDone));
			pass[RTX_PASS_GPU] = (float)(ts[TS_COPIED] - ts[TS_START]) * R.timestampPeriodNs / 1e6f;
			for (int i = 0; i < RTX_PASS_COUNT; i++)
			{
				R.statPass[i] += pass[i];
				R.hdr->passMs[i] = (R.hdr->passMs[i] > 0.0f) ? 0.9f * R.hdr->passMs[i] + 0.1f * pass[i] : pass[i];
			}
			if ((++R.statFrames >= 300) && getenv("CTR_RTX_DEBUG"))
			{
				logf_("trace pass: %.2f ms average, %.3f ms per sample per pixel (%u frames, %ux%u)", R.statTraceMs / R.statFrames, R.statSppMs / R.statFrames,
				      R.statFrames, R.inW, R.inH);
				double n = R.statFrames;
				logf_("passes (ms): build %.2f, trace %.2f, cache %.2f, a-trous %.2f, final %.2f, copy %.2f | GPU %.2f, CPU %.2f", R.statPass[RTX_PASS_BUILD] / n,
				      R.statPass[RTX_PASS_TRACE] / n, R.statPass[RTX_PASS_CACHE] / n, R.statPass[RTX_PASS_ATROUS] / n, R.statPass[RTX_PASS_FINAL] / n,
				      R.statPass[RTX_PASS_COPY] / n, R.statPass[RTX_PASS_GPU] / n, R.statPass[RTX_PASS_CPU] / n);
				R.statTraceMs = R.statSppMs = 0.0;
				memset(R.statPass, 0, sizeof(R.statPass));
				R.statFrames = 0;
			}
			R.traceMsPerSpp = (R.traceMsPerSpp > 0.0f) ? 0.8f * R.traceMsPerSpp + 0.2f * per : per;
			float want = R.budgetMs / R.traceMsPerSpp;
			uint32_t next = (uint32_t)fmaxf(1.0f, fminf((float)preset, floorf(want)));
			// change gradually: no visible pumping
			if (next > R.spp * 2)
			{
				next = R.spp * 2;
			}
			if (next < R.spp / 2)
			{
				next = R.spp / 2 > 0 ? R.spp / 2 : 1;
			}
			R.spp = next;
			R.hdr->traceSpp = R.spp;
		}
	}

	memcpy(R.prevR, H->camR, sizeof(R.prevR));
	memcpy(R.prevT, H->camT, sizeof(R.prevT));
	memcpy(R.prevCamPos, camPos, sizeof(camPos));
	R.prevH = H->projH;
	R.havePrev = 1;
	R.lastGameFrame = H->frame;
	R.parity ^= 1;
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: ctr_rtx <shm name> (started by ctr_native)\n");
		return 1;
	}
	{
		strncpy(s_exeDir, argv[0], sizeof(s_exeDir) - 1);
		char *slash = strrchr(s_exeDir, '/');
		if (slash)
		{
			*slash = 0;
		}
		else
		{
			strcpy(s_exeDir, ".");
		}
	}

	int fd = shm_open(argv[1], O_RDWR, 0600);
	if (fd < 0)
	{
		logf_("shm_open %s: %s", argv[1], strerror(errno));
		return 1;
	}
	R.shm = mmap(NULL, RTX_SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (R.shm == MAP_FAILED)
	{
		logf_("mmap: %s", strerror(errno));
		return 1;
	}
	shm_unlink(argv[1]); // both sides have it mapped; nothing leaks if either crashes
	R.hdr = (struct RtxHeader *)R.shm;
	if ((R.hdr->magic != RTX_MAGIC) || (R.hdr->version != RTX_VERSION))
	{
		logf_("shared memory version mismatch");
		return 1;
	}

	initVulkan();
	initPipelines();
	initFrameBuffers();
	R.hdr->rendererPid = (uint32_t)getpid();
	__sync_synchronize();
	R.hdr->rendererReady = 1;
	logf_("ready");

	uint32_t lastDone = R.hdr->frameReq;
	int setsDirty = 1;
	double idleSince = nowMs();
	while (!R.hdr->quit)
	{
		uint32_t req = R.hdr->frameReq;
		if (req == lastDone)
		{
			if (nowMs() - idleSince > 1000.0)
			{
				if (kill((pid_t)R.hdr->gamePid, 0) != 0)
				{
					break;
				}
				idleSince = nowMs();
			}
			futexWait(&R.hdr->frameReq, req, 20); // (the game wakes it)
			continue;
		}
		idleSince = nowMs();
		__sync_synchronize();
		double t0 = nowMs();

		if (R.hdr->levelGen != R.levelGen || !R.haveLevel)
		{
			R.hdr->rendererBusy = 1;
			loadLevel();
			setsDirty = 1;
		}
		uint32_t w = R.hdr->outW, h = R.hdr->outH;
		if (w < 8)
		{
			w = 8;
		}
		if (h < 8)
		{
			h = 8;
		}
		// traced size: the output scaled (TAA upscales it), at least 8x8
		uint32_t scale = R.hdr->settings.renderScale;
		scale = (scale < 25u) ? 100u : ((scale > 100u) ? 100u : scale);
		uint32_t inW = (w * scale + 50u) / 100u, inH = (h * scale + 50u) / 100u;
		inW = (inW < 8u) ? 8u : inW;
		inH = (inH < 8u) ? 8u : inH;
		if ((w != R.outW) || (h != R.outH) || (inW != R.inW) || (inH != R.inH))
		{
			resize(inW, inH, w, h);
			setsDirty = 1;
		}
		if (setsDirty)
		{
			vkDeviceWaitIdle(R.dev);
			writeSets();
			setsDirty = 0;
		}
		double tSetup = nowMs();
		if (R.levelPrep)
		{
			R.hdr->rendererBusy = 1;
			levelPrep();
		}
		R.hdr->rendererBusy = 0;

		double tPrep = nowMs();
		renderFrame();
		R.hdr->renderMs = (float)(nowMs() - t0);
		if (R.hdr->renderMs > 200.0f)
		{
			// (the game gives up waiting after 250 ms unless rendererBusy)
			logf_("slow request %u: %.0f ms (level load / resize %.0f, level prep %.0f, frame %.0f)", req, R.hdr->renderMs, tSetup - t0, tPrep - tSetup, nowMs() - tPrep);
		}
		__sync_synchronize();
		R.hdr->frameDone = req;
		futexWake(&R.hdr->frameDone);
		lastDone = req;
	}
	logf_("exit");
	return 0;
}
