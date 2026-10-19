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
#include <sys/mman.h>
#include <sys/stat.h>
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
	B_COUNT
};

enum
{
	PASS_TRACE,
	PASS_ATROUS1, // .. PASS_ATROUS1 + NUM_ATROUS - 1
	PASS_FINAL = PASS_ATROUS1 + 5,
	PASS_COUNT
};
#define NUM_ATROUS 5

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

	VkDescriptorSetLayout setLayout;
	VkPipelineLayout pipeLayout;
	VkDescriptorPool descPool;
	VkDescriptorSet sets[2][PASS_COUNT];
	VkPipeline pipeTrace, pipeAtrous, pipeFinal, pipeLights;

	// shared memory
	uint8_t *shm;
	struct RtxHeader *hdr;

	// level
	uint32_t levelGen;
	int haveLevel;
	Buf levelVerts, levelTris, levelIdx, lights, lightsStage, levelNormals;
	Accel levelBlas;
	uint32_t numLights; // static: lights file lamps, then glowing level surfaces
	uint32_t numLamps;  // lamps from the lights file (full relight only)

	// per frame
	Buf frame, texSlots, texSlotsStage, dynTris, dynTrisStage, dynPos, dynPosStage, dynNormals, dynNormalsStage, vram, vramStage, tlasInstances, scratch, lum;
	Accel dynBlas, tlas;

	// per pixel
	uint32_t outW, outH;
	Buf albedo, geo[2], hist[2], filterA, filterB, layer, out, readback;
	uint32_t parity;
	uint32_t frameIndex;

	// lighting
	struct RtxLighting light;
	float prevR[9], prevT[3], prevH, prevCamPos[3];
	int havePrev;
	uint32_t lastGameFrame;
	uint32_t lastLightMode;
	float autoExposure;
} R;

// ---- small helpers -----------------------------------------------------------

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
	L->sunCone = 1.0f * (float)M_PI / 180.0f;
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

// assets/rtx_lights/<key>.txt, written by scripts/rt_bake.py --export-lights:
//   indoor 0|1
//   sun dx dy dz cone r g b
//   sky zr zg zb hr hg hb
//   ground r g b
//   ambient r g b
//   exposure e
//   light x y z height r g b falloff size
static void loadLighting(const char *key)
{
	defaultLighting(&R.light);
	char path[1400];
	snprintf(path, sizeof(path), "%s/assets/rtx_lights/%s.txt", s_exeDir, key);
	FILE *f = fopen(path, "r");
	if (!f)
	{
		logf_("no %s, default outdoor lighting", path);
		return;
	}
	char line[512];
	struct RtxLighting *L = &R.light;
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

// area-weighted face normal added with the sign that agrees with what the
// vertex has so far (PS1 meshes do not keep one winding)
static void accumulateNormal(float *acc, const float *n)
{
	float sign = (acc[0] * n[0] + acc[1] * n[1] + acc[2] * n[2] < 0.0f) ? -1.0f : 1.0f;
	acc[0] += sign * n[0];
	acc[1] += sign * n[1];
	acc[2] += sign * n[2];
}

static void faceNormal(const float *p0, const float *p1, const float *p2, float *n)
{
	float e1[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]}, e2[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
	cross3(e1, e2, n);
}

// models: weld corners at the same position within one object
static void dynSmoothNormals(const struct RtxDynTri *tris, uint32_t n, float *out)
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
				accumulateNormal(acc[h], fn);
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

// additive-blended level surfaces (lamps, lava, torch flames built into the
// level): averaged per 400-unit cell into point lights with their own colour
static void addLevelEmitters(const struct RtxLevelVert *verts, const struct RtxLevelTri *tris, uint32_t nt)
{
	const struct RtxTexSlot *slots = (const void *)(R.shm + RTX_OFF_TEX_SLOTS);
	enum
	{
		MAXC = 512
	};
	static int32_t cell[MAXC][3];
	static float sum[MAXC][7]; // pos*w, color*w, w
	int nc = 0;
	for (uint32_t i = 0; i < nt; i++)
	{
		const struct RtxLevelTri *t = &tris[i];
		if (!(t->flags & RTX_TRI_TEXTURED) || (t->flags & RTX_TRI_WATER) || (t->texSlot >= R.hdr->numTexSlots))
		{
			continue;
		}
		const struct RtxTexSlot *sl = &slots[t->texSlot];
		if (((sl->tpage >> 5) & 3) != 1)
		{
			continue;
		}
		const float *p[3] = {verts[t->v[0]].pos, verts[t->v[1]].pos, verts[t->v[2]].pos};
		float fn[3];
		faceNormal(p[0], p[1], p[2], fn);
		float area = 0.5f * sqrtf(fn[0] * fn[0] + fn[1] * fn[1] + fn[2] * fn[2]);
		float tex[3] = {0, 0, 0}, rgb[3];
		int count = 0;
		for (int c = 0; c < 3; c++)
		{
			const uint8_t *uv = sl->uv[t->corner[c]];
			if (cpuTexel(sl->tpage, sl->clut, uv[0], uv[1], rgb))
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
		float col[3] = {0, 0, 0};
		for (int c = 0; c < 3; c++)
		{
			uint32_t rgba = verts[t->v[c]].rgba;
			col[0] += (rgba & 255) / 128.0f / 3.0f * tex[0] / count;
			col[1] += ((rgba >> 8) & 255) / 128.0f / 3.0f * tex[1] / count;
			col[2] += ((rgba >> 16) & 255) / 128.0f / 3.0f * tex[2] / count;
		}
		float mx = fmaxf(col[0], fmaxf(col[1], col[2])), mn = fminf(col[0], fminf(col[1], col[2]));
		if ((luma3(col) < 0.25f) || (mx - mn < 0.3f * mx)) // dim sheets and white foam/sparkle are not light sources
		{
			continue;
		}
		float centre[3] = {(p[0][0] + p[1][0] + p[2][0]) / 3, (p[0][1] + p[1][1] + p[2][1]) / 3, (p[0][2] + p[1][2] + p[2][2]) / 3};
		int32_t key[3] = {(int32_t)floorf(centre[0] / 400.0f), (int32_t)floorf(centre[1] / 400.0f), (int32_t)floorf(centre[2] / 400.0f)};
		int c = 0;
		while ((c < nc) && !((cell[c][0] == key[0]) && (cell[c][1] == key[1]) && (cell[c][2] == key[2])))
		{
			c++;
		}
		if (c == nc)
		{
			if (nc == MAXC)
			{
				continue;
			}
			memcpy(cell[nc], key, sizeof(key));
			memset(sum[nc], 0, sizeof(sum[nc]));
			nc++;
		}
		float w = area + 1.0f;
		for (int k = 0; k < 3; k++)
		{
			sum[c][k] += centre[k] * w;
			sum[c][3 + k] += col[k] * w;
		}
		sum[c][6] += w;
	}
	int added = 0;
	for (int c = 0; (c < nc) && (R.light.numLights < RTX_MAX_LIGHTS); c++)
	{
		struct RtxPointLight *l = &R.light.lights[R.light.numLights++];
		float w = sum[c][6];
		float strength = 1.2f + 0.4f * logf(1.0f + w / 20000.0f);
		for (int k = 0; k < 3; k++)
		{
			l->pos[k] = sum[c][k] / w;
			l->color[k] = sum[c][3 + k] / w * strength;
		}
		l->pos[1] += 30.0f; // just off the glowing surface
		l->height = 0.0f;
		l->falloff = 700.0f;
		l->size = 60.0f;
		added++;
	}
	if (added)
	{
		logf_("%d glowing level surfaces -> point lights", added);
	}
}

// ---- level ---------------------------------------------------------------------------

static void loadLevel(void)
{
	vkDeviceWaitIdle(R.dev);
	bufDestroy(&R.levelVerts);
	bufDestroy(&R.levelTris);
	bufDestroy(&R.levelIdx);
	bufDestroy(&R.lights);
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
	R.levelTris = uploadBuf(tris, sizeof(*tris) * nt, 0);
	R.levelIdx = uploadBuf(idx, sizeof(uint32_t) * 3 * nt, USAGE_AS_INPUT);
	free(idx);

	float *nrm = calloc((size_t)(nv ? nv : 1) * 4, sizeof(float));
	for (uint32_t i = 0; i < nt; i++)
	{
		float fn[3];
		faceNormal(verts[tris[i].v[0]].pos, verts[tris[i].v[1]].pos, verts[tris[i].v[2]].pos, fn);
		for (int c = 0; c < 3; c++)
		{
			accumulateNormal(&nrm[tris[i].v[c] * 4], fn);
		}
	}
	R.levelNormals = uploadBuf(nrm, sizeof(float) * 4 * (nv ? nv : 1), 0);
	free(nrm);

	cmdBegin();
	VkAccelerationStructureGeometryKHR g = triGeometry(R.levelVerts.addr, sizeof(struct RtxLevelVert), nv ? nv - 1 : 0, R.levelIdx.addr);
	buildBlas(&R.levelBlas, &g, nt, nt, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
	cmdSubmitWait();

	loadLighting(R.hdr->lightsKey);
	R.numLamps = R.light.numLights;
	addLevelEmitters(verts, tris, nt);
	R.numLights = R.light.numLights;
	// static lights, then room for this frame's emitters (RTX_MAX_DYN_LIGHTS)
	float *lightData = calloc((R.numLights + RTX_MAX_DYN_LIGHTS) * 12, sizeof(float));
	for (uint32_t i = 0; i < R.numLights; i++)
	{
		const struct RtxPointLight *pl = &R.light.lights[i];
		float *d = &lightData[i * 12];
		d[0] = pl->pos[0];
		d[1] = pl->pos[1];
		d[2] = pl->pos[2];
		d[3] = pl->height;
		d[4] = pl->color[0];
		d[5] = pl->color[1];
		d[6] = pl->color[2];
		d[7] = pl->falloff;
		d[8] = pl->size;
	}
	R.lights = uploadBuf(lightData, sizeof(float) * 12 * (R.numLights + RTX_MAX_DYN_LIGHTS), 0);
	free(lightData);

	R.haveLevel = 1;
	R.havePrev = 0;
	R.autoExposure = 0.0f;
	R.levelGen = R.hdr->levelGen;
	logf_("level %d: %u verts, %u tris", R.hdr->levelID, nv, nt);
}

// ---- per-pixel buffers ------------------------------------------------------------------

static void resize(uint32_t w, uint32_t h)
{
	vkDeviceWaitIdle(R.dev);
	Buf *bufs[] = {&R.albedo, &R.geo[0], &R.geo[1], &R.hist[0], &R.hist[1], &R.filterA, &R.filterB, &R.layer};
	for (unsigned i = 0; i < sizeof(bufs) / sizeof(*bufs); i++)
	{
		bufDestroy(bufs[i]);
		*bufs[i] = bufCreate((VkDeviceSize)w * h * 16, USAGE_STORAGE, DEVICE_LOCAL);
	}
	bufDestroy(&R.out);
	bufDestroy(&R.readback);
	R.out = bufCreate((VkDeviceSize)w * h * 4, USAGE_STORAGE, DEVICE_LOCAL);
	R.readback = bufCreate((VkDeviceSize)w * h * 4, VK_BUFFER_USAGE_TRANSFER_DST_BIT, HOST_MEM | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	// geo/hist start invalid
	cmdBegin();
	for (int i = 0; i < 2; i++)
	{
		vkCmdFillBuffer(R.cmd, R.geo[i].buf, 0, VK_WHOLE_SIZE, 0);
		vkCmdFillBuffer(R.cmd, R.hist[i].buf, 0, VK_WHOLE_SIZE, 0);
	}
	cmdSubmitWait();
	R.outW = w;
	R.outH = h;
	R.havePrev = 0;
	logf_("output %ux%u", w, h);
}

static void writeSets(void)
{
	for (int par = 0; par < 2; par++)
	{
		Buf *geoCur = &R.geo[par], *geoPrev = &R.geo[par ^ 1], *histCur = &R.hist[par], *histPrev = &R.hist[par ^ 1];
		for (int pass = 0; pass < PASS_COUNT; pass++)
		{
			// a-trous k (1-based) ping-pongs histCur -> A -> B -> A ...; final reads A
			Buf *src = histCur, *dst = &R.filterA;
			if ((pass > PASS_ATROUS1) && (pass < PASS_FINAL))
			{
				int k = pass - PASS_ATROUS1 + 1;
				src = (k % 2 == 0) ? &R.filterA : &R.filterB;
				dst = (k % 2 == 1) ? &R.filterA : &R.filterB;
			}
			else if (pass == PASS_FINAL)
			{
				src = &R.filterA;
				dst = &R.filterB;
			}
			Buf *b[B_COUNT] = {NULL,    &R.frame, &R.levelVerts, &R.levelTris, &R.texSlots, &R.dynTris,        &R.vram,        &R.lights, &R.albedo, geoCur,
			                   geoPrev, histPrev, histCur,       src,          dst,         &R.out,            &R.lum,         &R.levelNormals, &R.dynNormals, &R.layer};
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

static void initFrameBuffers(void)
{
	R.frame = bufCreate(sizeof(FrameData), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, HOST_MEM);
	R.texSlots = bufCreate(RTX_MAX_TEX_SLOTS * sizeof(struct RtxTexSlot), USAGE_STORAGE, DEVICE_LOCAL);
	R.texSlotsStage = bufCreate(RTX_MAX_TEX_SLOTS * sizeof(struct RtxTexSlot), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.dynTris = bufCreate(RTX_MAX_DYN_TRIS * sizeof(struct RtxDynTri), USAGE_STORAGE, DEVICE_LOCAL);
	R.dynTrisStage = bufCreate(RTX_MAX_DYN_TRIS * sizeof(struct RtxDynTri), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.dynPos = bufCreate(RTX_MAX_DYN_TRIS * 36, USAGE_STORAGE | USAGE_AS_INPUT, DEVICE_LOCAL);
	R.dynPosStage = bufCreate(RTX_MAX_DYN_TRIS * 36, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.dynNormals = bufCreate(RTX_MAX_DYN_TRIS * 48, USAGE_STORAGE, DEVICE_LOCAL);
	R.dynNormalsStage = bufCreate(RTX_MAX_DYN_TRIS * 48, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.lightsStage = bufCreate(RTX_MAX_DYN_LIGHTS * 48, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.vram = bufCreate(RTX_VRAM_W * RTX_VRAM_H * 2, USAGE_STORAGE, DEVICE_LOCAL);
	R.vramStage = bufCreate(RTX_VRAM_W * RTX_VRAM_H * 2, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, HOST_MEM);
	R.tlasInstances = bufCreate(2 * sizeof(VkAccelerationStructureInstanceKHR), USAGE_AS_INPUT, HOST_MEM);
	R.lum = bufCreate(16, USAGE_STORAGE, HOST_MEM);

	// TLAS sized for 2 instances, built every frame
	VkAccelerationStructureGeometryKHR g = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR};
	g.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	g.geometry.instances.data.deviceAddress = R.tlasInstances.addr;
	VkAccelerationStructureBuildGeometryInfoKHR bi = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	                                                   .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	                                                   .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	                                                   .geometryCount = 1,
	                                                   .pGeometries = &g};
	uint32_t maxInst = 2;
	VkAccelerationStructureBuildSizesInfoKHR sizes = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	vkGetAccelerationStructureBuildSizesKHR(R.dev, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &bi, &maxInst, &sizes);
	accelCreate(&R.tlas, VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, sizes.accelerationStructureSize);
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

static void renderFrame(void)
{
	struct RtxHeader *H = R.hdr;
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
	F->size[0] = R.outW;
	F->size[1] = R.outH;
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
	// exposure: open sunlit ground (or the indoor ambient) maps to the PS1 colour
	float exposure = L->exposure;
	if (exposure <= 0.0f)
	{
		if (L->indoor)
		{
			exposure = (R.autoExposure > 0.0f) ? R.autoExposure : 1.0f / (luma3(L->ambient) + 0.3f);
		}
		else
		{
			float open[3];
			for (int i = 0; i < 3; i++)
			{
				open[i] = L->sunColor[i] * F->sunDir[1] + 0.2f * L->skyHorizon[i] + 0.8f * L->skyZenith[i];
			}
			exposure = 1.0f / luma3(open);
		}
	}
	int hybrid = (H->lightMode == 0);
	F->sunColor[3] = hybrid ? 1.0f : exposure; // hybrid: the PS1 colours as they are
	F->skyZenith[3] = 0.35f;   // level vertex colour kept as tint
	F->skyHorizon[3] = 2.0f;   // ray epsilon (level units)
	F->groundColor[3] = 32.0f; // max accumulated frames
	F->ambient[3] = L->indoor ? 1.0f : 0.0f;
	F->counts[0] = R.numLights;
	F->counts[1] = H->numLevelTris;
	F->counts[2] = nd;
	F->counts[3] = reset ? 1u : 0u;
	F->params[0] = 1500.0f; // indoor AO radius
	F->params[1] = (float)R.frameIndex / 30.0f; // seconds, water ripples
	uint32_t ndl = H->numDynLights > RTX_MAX_DYN_LIGHTS ? RTX_MAX_DYN_LIGHTS : H->numDynLights;
	F->extra[0] = ndl;
	F->extra[1] = R.numLamps;
	F->extra[2] = H->lightMode;

	// ---- uploads
	memcpy(R.texSlotsStage.map, R.shm + RTX_OFF_TEX_SLOTS, H->numTexSlots * sizeof(struct RtxTexSlot));
	memcpy(R.vramStage.map, R.shm + RTX_OFF_VRAM, RTX_VRAM_W * RTX_VRAM_H * 2);
	const struct RtxDynTri *dyn = (const void *)(R.shm + RTX_OFF_DYN_TRIS);
	memcpy(R.dynTrisStage.map, dyn, nd * sizeof(struct RtxDynTri));
	float *dp = R.dynPosStage.map;
	for (uint32_t i = 0; i < nd; i++)
	{
		memcpy(&dp[i * 9], dyn[i].p, 36);
	}
	dynSmoothNormals(dyn, nd, R.dynNormalsStage.map);
	float *ls = R.lightsStage.map;
	for (uint32_t i = 0; i < ndl; i++)
	{
		const struct RtxPointLight *l = &H->dynLights[i];
		float rec[12] = {l->pos[0], l->pos[1], l->pos[2], 0.0f, l->color[0], l->color[1], l->color[2], l->falloff, l->size, 0, 0, 0};
		memcpy(&ls[i * 12], rec, sizeof(rec));
	}

	VkAccelerationStructureInstanceKHR inst[2];
	memset(inst, 0, sizeof(inst));
	uint32_t numInst = 0;
	for (int k = 0; k < 2; k++)
	{
		if (k == 1 && nd == 0)
		{
			break;
		}
		VkAccelerationStructureInstanceKHR *I = &inst[numInst++];
		I->transform.matrix[0][0] = I->transform.matrix[1][1] = I->transform.matrix[2][2] = 1.0f;
		I->instanceCustomIndex = (uint32_t)k;
		I->mask = (k == 0) ? 0x1 : 0x2;
		I->accelerationStructureReference = (k == 0) ? R.levelBlas.addr : R.dynBlas.addr;
	}

	cmdBegin();
	copyBuf(&R.texSlotsStage, &R.texSlots, H->numTexSlots * sizeof(struct RtxTexSlot));
	copyBuf(&R.vramStage, &R.vram, RTX_VRAM_W * RTX_VRAM_H * 2);
	copyBuf(&R.dynTrisStage, &R.dynTris, nd * sizeof(struct RtxDynTri));
	copyBuf(&R.dynPosStage, &R.dynPos, nd * 36);
	copyBuf(&R.dynNormalsStage, &R.dynNormals, nd * 48);
	if (ndl > 0)
	{
		VkBufferCopy lc = {0, (VkDeviceSize)R.numLights * 48, (VkDeviceSize)ndl * 48};
		vkCmdCopyBuffer(R.cmd, R.lightsStage.buf, R.lights.buf, 1, &lc);
	}
	barrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);

	if (nd > 0)
	{
		VkAccelerationStructureGeometryKHR g = triGeometry(R.dynPos.addr, 12, nd * 3 - 1, 0);
		buildBlas(&R.dynBlas, &g, nd, RTX_MAX_DYN_TRIS, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR);
		inst[1].accelerationStructureReference = R.dynBlas.addr;
		// BLAS -> TLAS (also orders the shared scratch buffer)
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
		uint32_t maxInst = 2;
		VkAccelerationStructureBuildSizesInfoKHR sizes = {VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
		vkGetAccelerationStructureBuildSizesKHR(R.dev, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &bi, &maxInst, &sizes);
		bi.scratchData.deviceAddress = scratchAddr(sizes.buildScratchSize);
		VkAccelerationStructureBuildRangeInfoKHR range = {.primitiveCount = numInst};
		const VkAccelerationStructureBuildRangeInfoKHR *pr = &range;
		vkCmdBuildAccelerationStructuresKHR(R.cmd, 1, &bi, &pr);
	}
	barrier(VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
	        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_SHADER_READ_BIT);

	uint32_t gx = (R.outW + 7) / 8, gy = (R.outH + 7) / 8;
	VkDescriptorSet *sets = R.sets[R.parity];
	vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeTrace);
	vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &sets[PASS_TRACE], 0, NULL);
	vkCmdDispatch(R.cmd, gx, gy, 1);

	vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeAtrous);
	static const int steps[NUM_ATROUS] = {1, 2, 4, 8, 16};
	for (int i = 0; i < NUM_ATROUS; i++)
	{
		barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
		vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &sets[PASS_ATROUS1 + i], 0, NULL);
		vkCmdPushConstants(R.cmd, R.pipeLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4, &steps[i]);
		vkCmdDispatch(R.cmd, gx, gy, 1);
	}

	barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT);
	vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeFinal);
	vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &sets[PASS_FINAL], 0, NULL);
	vkCmdDispatch(R.cmd, gx, gy, 1);

	barrier(VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT);
	copyBuf(&R.out, &R.readback, (VkDeviceSize)R.outW * R.outH * 4);
	cmdSubmitWait();

	memcpy(R.shm + RTX_OFF_IMAGE, R.readback.map, (size_t)R.outW * R.outH * 4);

	memcpy(R.prevR, H->camR, sizeof(R.prevR));
	memcpy(R.prevT, H->camT, sizeof(R.prevT));
	memcpy(R.prevCamPos, camPos, sizeof(camPos));
	R.prevH = H->projH;
	R.havePrev = 1;
	R.lastGameFrame = H->frame;
	R.parity ^= 1;
}

static void liftLights(void)
{
	if (R.numLights == 0)
	{
		return;
	}
	// TLAS with the level only
	R.hdr->numDynTris = 0;
	cmdBegin();
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
	FrameData *F = R.frame.map;
	F->counts[0] = R.numLights;
	vkCmdBindPipeline(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLights);
	vkCmdBindDescriptorSets(R.cmd, VK_PIPELINE_BIND_POINT_COMPUTE, R.pipeLayout, 0, 1, &R.sets[0][PASS_TRACE], 0, NULL);
	vkCmdDispatch(R.cmd, (R.numLights + 63) / 64, 1, 1);
	cmdSubmitWait();
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
			struct timespec ts = {0, 50000};
			nanosleep(&ts, NULL);
			continue;
		}
		idleSince = nowMs();
		__sync_synchronize();
		double t0 = nowMs();

		if (R.hdr->levelGen != R.levelGen || !R.haveLevel)
		{
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
		if ((w != R.outW) || (h != R.outH))
		{
			resize(w, h);
			setsDirty = 1;
		}
		if (setsDirty)
		{
			vkDeviceWaitIdle(R.dev);
			writeSets();
			liftLights();
			setsDirty = 0;
		}

		renderFrame();
		R.hdr->renderMs = (float)(nowMs() - t0);
		__sync_synchronize();
		R.hdr->frameDone = req;
		lastDone = req;
	}
	logf_("exit");
	return 0;
}
