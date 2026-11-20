#include <vulkan/vulkan.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#define CHECK(call) do { \
    VkResult result_ = (call); \
    if (result_ != VK_SUCCESS) { \
        std::fprintf(stderr, "%s failed: VkResult=%d at line %d\n", #call, result_, __LINE__); \
        return 1; \
    } \
} while (0)

int main(int argc, char **argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s ABSOLUTE_OR_RELATIVE_ICD_MANIFEST.json\n", argv[0]);
        return 2;
    }
    if (setenv("VK_ICD_FILENAMES", argv[1], 1) != 0) {
        std::perror("setenv VK_ICD_FILENAMES");
        return 1;
    }

    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "vkgl-lg110-smoke";
    app.apiVersion = VK_API_VERSION_1_0;
    VkInstanceCreateInfo instance_info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instance_info.pApplicationInfo = &app;
    VkInstance instance = VK_NULL_HANDLE;
    CHECK(vkCreateInstance(&instance_info, nullptr, &instance));

    uint32_t count = 0;
    CHECK(vkEnumeratePhysicalDevices(instance, &count, nullptr));
    if (count == 0) {
        std::fprintf(stderr, "no Vulkan physical device exposed by the selected ICD\n");
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    std::vector<VkPhysicalDevice> devices(count);
    CHECK(vkEnumeratePhysicalDevices(instance, &count, devices.data()));

    VkPhysicalDevice selected = VK_NULL_HANDLE;
    for (VkPhysicalDevice device : devices) {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device, &properties);
        std::printf("DEVICE name=%s api=%u.%u.%u vendor=0x%04x device=0x%04x\n",
                    properties.deviceName, VK_VERSION_MAJOR(properties.apiVersion),
                    VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion),
                    properties.vendorID, properties.deviceID);
        if (std::strstr(properties.deviceName, "LoongGPU") &&
            !std::strstr(properties.deviceName, "llvmpipe")) {
            selected = device;
        }
    }
    if (selected == VK_NULL_HANDLE) {
        std::fprintf(stderr, "selected ICD did not expose a non-llvmpipe LoongGPU device\n");
        vkDestroyInstance(instance, nullptr);
        return 1;
    }

    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(selected, &queue_family_count, nullptr);
    if (queue_family_count == 0) {
        std::fprintf(stderr, "LoongGPU exposes no queue family\n");
        vkDestroyInstance(instance, nullptr);
        return 1;
    }
    float priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &priority;
    VkDeviceCreateInfo device_info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;
    VkDevice device = VK_NULL_HANDLE;
    CHECK(vkCreateDevice(selected, &device_info, nullptr, &device));
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    std::puts("PASS LG110_VULKAN_SMOKE instance=1 physical=1 device=1");
    return 0;
}
