// Checks whether this process (32-bit) can use Vulkan ray tracing on the GPU.
#define VOLK_IMPLEMENTATION
#include "volk.h"
#include <stdio.h>
#include <string.h>

static const char *want[] = {
	"VK_KHR_acceleration_structure", "VK_KHR_ray_query", "VK_KHR_ray_tracing_pipeline",
	"VK_KHR_deferred_host_operations", "VK_KHR_buffer_device_address",
};

int main(void)
{
	if (volkInitialize() != VK_SUCCESS) { puts("no vulkan loader"); return 1; }
	VkApplicationInfo app = {VK_STRUCTURE_TYPE_APPLICATION_INFO, .apiVersion = VK_API_VERSION_1_3};
	VkInstanceCreateInfo ici = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app};
	VkInstance inst;
	if (vkCreateInstance(&ici, 0, &inst) != VK_SUCCESS) { puts("vkCreateInstance failed"); return 1; }
	volkLoadInstance(inst);
	uint32_t n = 8; VkPhysicalDevice devs[8];
	vkEnumeratePhysicalDevices(inst, &n, devs);
	int ok_any = 0;
	for (uint32_t i = 0; i < n; i++) {
		VkPhysicalDeviceProperties p; vkGetPhysicalDeviceProperties(devs[i], &p);
		uint32_t ne = 0; vkEnumerateDeviceExtensionProperties(devs[i], 0, &ne, 0);
		static VkExtensionProperties ext[1024]; if (ne > 1024) ne = 1024;
		vkEnumerateDeviceExtensionProperties(devs[i], 0, &ne, ext);
		printf("%s (api %u.%u, %u exts, ptr %zu-bit)\n", p.deviceName, VK_API_VERSION_MAJOR(p.apiVersion),
		       VK_API_VERSION_MINOR(p.apiVersion), ne, sizeof(void *) * 8);
		int ok = 1;
		for (unsigned w = 0; w < sizeof want / sizeof *want; w++) {
			int has = 0;
			for (uint32_t e = 0; e < ne; e++) has |= !strcmp(ext[e].extensionName, want[w]);
			printf("  %-34s %s\n", want[w], has ? "yes" : "NO");
			ok &= has;
		}
		VkPhysicalDeviceRayQueryFeaturesKHR rq = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};
		VkPhysicalDeviceAccelerationStructureFeaturesKHR as = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR, &rq};
		VkPhysicalDeviceVulkan12Features v12 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES, &as};
		VkPhysicalDeviceFeatures2 f2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, &v12};
		if (ok) {
			vkGetPhysicalDeviceFeatures2(devs[i], &f2);
			printf("  features: accelStruct=%u rayQuery=%u bufferDeviceAddress=%u\n",
			       as.accelerationStructure, rq.rayQuery, v12.bufferDeviceAddress);
			ok = as.accelerationStructure && rq.rayQuery && v12.bufferDeviceAddress;
		}
		ok_any |= ok;
	}
	puts(ok_any ? "RT: OK" : "RT: UNAVAILABLE");
	return !ok_any;
}
