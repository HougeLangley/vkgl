#include <vulkan/vulkan.h>
#include <gbm.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#define W 5u
#define H 4u
#define BYTES (W * H * 4u)
#define CHECK(c, ...) do { if (!(c)) { std::fprintf(stderr, "FAIL line=%d ", __LINE__); std::fprintf(stderr, __VA_ARGS__); std::fprintf(stderr, "\n"); throw std::runtime_error("check"); } } while (0)
#define VKCHECK(c, call) do { VkResult r_ = (call); CHECK((c), "%s = %d", #call, r_); } while (0)

static bool g_positive_only = false;

static const char *res_name(VkResult r) {
    switch (r) {
    case VK_SUCCESS: return "SUCCESS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "FORMAT_NOT_SUPPORTED";
    case VK_ERROR_INVALID_EXTERNAL_HANDLE: return "INVALID_EXTERNAL_HANDLE";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "FEATURE_NOT_PRESENT";
    default: return "OTHER";
    }
}

struct Api {
    PFN_vkGetPhysicalDeviceFeatures2KHR GetPhysicalDeviceFeatures2KHR{};
    PFN_vkGetPhysicalDeviceFormatProperties2KHR GetPhysicalDeviceFormatProperties2KHR{};
    PFN_vkGetPhysicalDeviceImageFormatProperties2KHR GetPhysicalDeviceImageFormatProperties2KHR{};
    PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR GetPhysicalDeviceExternalBufferPropertiesKHR{};
    PFN_vkGetImageMemoryRequirements2KHR GetImageMemoryRequirements2KHR{};
    PFN_vkGetBufferMemoryRequirements2KHR GetBufferMemoryRequirements2KHR{};
    PFN_vkGetImageSparseMemoryRequirements2KHR GetImageSparseMemoryRequirements2KHR{};
    PFN_vkGetMemoryFdPropertiesKHR GetMemoryFdPropertiesKHR{};
    PFN_vkGetMemoryFdKHR GetMemoryFdKHR{};
    PFN_vkGetImageDrmFormatModifierPropertiesEXT GetImageDrmFormatModifierPropertiesEXT{};
    PFN_vkBindImageMemory2KHR BindImageMemory2KHR{};
    PFN_vkBindBufferMemory2KHR BindBufferMemory2KHR{};
    PFN_vkCreateSamplerYcbcrConversionKHR CreateSamplerYcbcrConversionKHR{};
    PFN_vkDestroySamplerYcbcrConversionKHR DestroySamplerYcbcrConversionKHR{};
    PFN_vkTrimCommandPoolKHR TrimCommandPoolKHR{};
};

static void fill_pattern(uint8_t *p, VkFormat format) {
    for (unsigned y = 0; y < H; ++y) for (unsigned x = 0; x < W; ++x) {
        uint8_t r = uint8_t(32 + x * 17 + y * 3), g = uint8_t(64 + x * 5 + y * 29);
        uint8_t b = uint8_t(96 + x * 31 + y * 7), *q = p + (y * W + x) * 4;
        if (format == VK_FORMAT_B8G8R8A8_UNORM) { q[0] = b; q[1] = g; q[2] = r; }
        else { q[0] = r; q[1] = g; q[2] = b; }
        q[3] = 255;
    }
}

static VkImageCreateInfo image_info(VkFormat format, VkImageUsageFlags usage, void *pNext) {
    VkImageCreateInfo ci{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    ci.pNext = pNext; ci.imageType = VK_IMAGE_TYPE_2D; ci.format = format;
    ci.extent = {W, H, 1}; ci.mipLevels = 1; ci.arrayLayers = 1;
    ci.samples = VK_SAMPLE_COUNT_1_BIT; ci.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
    ci.usage = usage; ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    return ci;
}

static void negative_create_tests(VkDevice dev, VkFormat format) {
    VkExternalMemoryImageCreateInfo ext{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
    ext.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    VkSubresourceLayout planes[2]{};
    planes[0].rowPitch = 256;
    VkImageDrmFormatModifierExplicitCreateInfoEXT mod{VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT};
    mod.pNext = &ext; mod.drmFormatModifier = 0; mod.drmFormatModifierPlaneCount = 1; mod.pPlaneLayouts = planes;
    auto run = [&](const char *name, VkImageCreateInfo &ci) {
        VkImage image = VK_NULL_HANDLE;
        VkResult r = vkCreateImage(dev, &ci, nullptr, &image);
        std::printf("NEG_CREATE name=%s result=%d(%s) image=%p\n", name, r, res_name(r), (void *)image);
        if (image != VK_NULL_HANDLE) vkDestroyImage(dev, image, nullptr);
        CHECK(r == VK_ERROR_FORMAT_NOT_SUPPORTED && image == VK_NULL_HANDLE, "negative create %s accepted", name);
    };
    {
        auto ci = image_info(VK_FORMAT_G8_B8R8_2PLANE_420_UNORM,
                             VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, &mod);
        run("nv12", ci);
    }
    { auto ci = image_info(format, VK_IMAGE_USAGE_TRANSFER_DST_BIT, &mod); run("usage_transfer_dst", ci); }
    { auto ci = image_info(format, VK_IMAGE_USAGE_SAMPLED_BIT, &mod); mod.drmFormatModifier = 1;
      run("modifier_1", ci); mod.drmFormatModifier = 0; }
    { auto ci = image_info(format, VK_IMAGE_USAGE_SAMPLED_BIT, &mod); mod.drmFormatModifierPlaneCount = 2;
      run("planes_2", ci); mod.drmFormatModifierPlaneCount = 1; }
    { auto ci = image_info(format, VK_IMAGE_USAGE_SAMPLED_BIT, &mod); planes[0].rowPitch = 16;
      run("short_stride", ci); planes[0].rowPitch = 256; }
    { auto ci = image_info(format, VK_IMAGE_USAGE_SAMPLED_BIT, &mod); planes[0].size = 1;
      run("nonzero_input_size", ci); planes[0].size = 0; }
    { auto ci = image_info(format, VK_IMAGE_USAGE_SAMPLED_BIT, &mod); planes[0].offset = 1;
      run("offset_1", ci); planes[0].offset = 0; }
    { auto ci = image_info(format, VK_IMAGE_USAGE_SAMPLED_BIT, &ext); run("missing_explicit_layout", ci); }
    { auto ci = image_info(format, VK_IMAGE_USAGE_SAMPLED_BIT, &mod); ci.pNext = &mod; ext.handleTypes = 0;
      run("missing_handle_type", ci); ext.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT; }
    { auto ci = image_info(format, VK_IMAGE_USAGE_SAMPLED_BIT, &mod); ci.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
      run("preinitialized_external_image", ci); }
}

static void run_positive(VkDevice dev, VkQueue queue, const Api &api, struct gbm_device *gbm,
                         VkFormat format, VkDeviceSize plane_offset, unsigned iteration) {
    /* LG110 GBM exposes XRGB8888 linear storage. The dma-buf has no format metadata;
     * Vulkan/EGL import attributes below define the external BGRA/RGBA interpretation. */
    const uint32_t gbm_format = GBM_FORMAT_XRGB8888;
    const uint32_t external_fourcc = format == VK_FORMAT_B8G8R8A8_UNORM ? GBM_FORMAT_ARGB8888 : GBM_FORMAT_ABGR8888;
    const unsigned bo_width = plane_offset == 64 ? W + 16 : W;
    const unsigned bo_height = plane_offset == 256 ? H + 1 : H;
    gbm_bo *bo = gbm_bo_create(gbm, bo_width, bo_height, gbm_format, GBM_BO_USE_LINEAR);
    CHECK(bo, "gbm_bo_create");
    uint8_t expected[BYTES]{}, readback[BYTES]{};
    fill_pattern(expected, format);
    uint32_t map_stride = 0; void *map_data = nullptr;
    void *map = gbm_bo_map(bo, 0, 0, bo_width, bo_height, GBM_BO_TRANSFER_WRITE, &map_stride, &map_data);
    CHECK(map, "gbm_bo_map");
    const uint32_t stride = gbm_bo_get_stride_for_plane(bo, 0);
    CHECK(stride == map_stride && stride >= W * 4, "stride mismatch");
    for (unsigned y = 0; y < H; ++y)
        std::memcpy(static_cast<uint8_t *>(map) + plane_offset + size_t(y) * map_stride,
                    expected + size_t(y) * W * 4, W * 4);
    gbm_bo_unmap(bo, map_data);
    map_data = nullptr; map_stride = 0;
    map = gbm_bo_map(bo, 0, 0, bo_width, bo_height, GBM_BO_TRANSFER_READ, &map_stride, &map_data);
    CHECK(map, "gbm_bo_map source readback");
    for (unsigned y = 0; y < H; ++y)
        CHECK(std::memcmp(static_cast<uint8_t *>(map) + plane_offset + size_t(y) * map_stride,
                          expected + size_t(y) * W * 4, W * 4) == 0, "CPU source row %u mismatch", y);
    gbm_bo_unmap(bo, map_data); map_data = nullptr;
    std::printf("CPU_SOURCE iter=%u exact=1 bytes=%u bo_width=%u bo_height=%u\n", iteration, BYTES, bo_width, bo_height);
    int fd = gbm_bo_get_fd(bo);
    CHECK(fd >= 0, "gbm_bo_get_fd");
    char procpath[64], target[256];
    std::snprintf(procpath, sizeof(procpath), "/proc/self/fd/%d", fd);
    ssize_t link_len = readlink(procpath, target, sizeof(target) - 1);
    CHECK(link_len > 0, "fd readlink"); target[link_len] = 0;
    std::printf("POSITIVE iter=%u format=%d bo_fourcc=0x%08x external_fourcc=0x%08x stride=%u offset=%llu fd=%d target=%s bo_width=%u bo_height=%u\n",
                iteration, format, gbm_format, external_fourcc, stride, (unsigned long long)plane_offset, fd, target, bo_width, bo_height);

    VkSubresourceLayout plane{};
    plane.offset = plane_offset; plane.rowPitch = stride;
    VkExternalMemoryImageCreateInfo ext{VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO};
    ext.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    VkImageDrmFormatModifierExplicitCreateInfoEXT mod{VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT};
    mod.pNext = &ext; mod.drmFormatModifier = 0; mod.drmFormatModifierPlaneCount = 1; mod.pPlaneLayouts = &plane;
    auto ci = image_info(format, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, &mod);
    VkImage image = VK_NULL_HANDLE;
    VKCHECK(r_ == VK_SUCCESS, vkCreateImage(dev, &ci, nullptr, &image));

    VkMemoryDedicatedRequirements dedicated{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS};
    VkMemoryRequirements2 req2{VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2}; req2.pNext = &dedicated;
    VkImageMemoryRequirementsInfo2 req_info{VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2}; req_info.image = image;
    api.GetImageMemoryRequirements2KHR(dev, &req_info, &req2);
    const VkDeviceSize plane_size = VkDeviceSize(stride) * (H - 1) + W * 4;
    const VkDeviceSize expected_size = plane_offset + plane_size;
    std::printf("REQ iter=%u size=%llu alignment=%llu type_bits=0x%x dedicated=%d/%d\n", iteration,
                (unsigned long long)req2.memoryRequirements.size,
                (unsigned long long)req2.memoryRequirements.alignment,
                req2.memoryRequirements.memoryTypeBits, dedicated.requiresDedicatedAllocation,
                dedicated.prefersDedicatedAllocation);
    CHECK(expected_size <= VkDeviceSize(stride) * bo_height, "required span exceeds BO");
    CHECK(req2.memoryRequirements.size == expected_size && req2.memoryRequirements.memoryTypeBits == 4 &&
          dedicated.requiresDedicatedAllocation && dedicated.prefersDedicatedAllocation, "external requirements");

    VkImageSubresource sub{VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT, 0, 0}; VkSubresourceLayout layout{};
    vkGetImageSubresourceLayout(dev, image, &sub, &layout);
    CHECK(layout.offset == plane_offset && layout.rowPitch == stride && layout.size == plane_size,
          "subresource layout offset=%llu pitch=%llu size=%llu", (unsigned long long)layout.offset,
          (unsigned long long)layout.rowPitch, (unsigned long long)layout.size);
    std::printf("LAYOUT iter=%u aspect=MEMORY_PLANE_0 offset=%llu size=%llu rowPitch=%llu\n",
                iteration, (unsigned long long)layout.offset, (unsigned long long)layout.size,
                (unsigned long long)layout.rowPitch);
    if (!g_positive_only) {
        VkImageSubresource wrong_sub{VK_IMAGE_ASPECT_COLOR_BIT, 0, 0};
        VkSubresourceLayout wrong_layout{1, 2, 3, 4, 5};
        vkGetImageSubresourceLayout(dev, image, &wrong_sub, &wrong_layout);
        CHECK(wrong_layout.offset == 0 && wrong_layout.size == 0 && wrong_layout.rowPitch == 0,
              "COLOR aspect incorrectly accepted for DRM modifier image");
        std::printf("NEG_LAYOUT iter=%u aspect=COLOR zeroed=1\n", iteration);
    }
    VkImageDrmFormatModifierPropertiesEXT modifier_props{VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_PROPERTIES_EXT};
    VKCHECK(r_ == VK_SUCCESS, api.GetImageDrmFormatModifierPropertiesEXT(dev, image, &modifier_props));
    CHECK(modifier_props.drmFormatModifier == 0, "image modifier");
    VkMemoryFdPropertiesKHR fd_props{VK_STRUCTURE_TYPE_MEMORY_FD_PROPERTIES_KHR};
    VKCHECK(r_ == VK_SUCCESS, api.GetMemoryFdPropertiesKHR(dev, VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT, fd, &fd_props));
    CHECK(fd_props.memoryTypeBits == 4, "fd memory types");

    VkMemoryDedicatedAllocateInfo ded{VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
    ded.image = image;
    VkImportMemoryFdInfoKHR import{VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR};
    import.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
    VkMemoryAllocateInfo ai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    ai.allocationSize = req2.memoryRequirements.size; ai.memoryTypeIndex = 2;

    /* Failure ownership: non-dedicated and wrong-size imports must retain fd. */
    for (int mode = 0; !g_positive_only && mode < 4; ++mode) {
        int test_fd = mode == 2 ? -1 : fcntl(fd, F_DUPFD_CLOEXEC, 0);
        CHECK(mode == 2 || test_fd >= 0, "dup negative fd");
        import.fd = test_fd;
        VkMemoryAllocateInfo bad = ai;
        if (mode == 0) { bad.pNext = &import; }
        else { ded.pNext = &import; bad.pNext = &ded; if (mode == 1) bad.allocationSize -= 4; if (mode == 3) bad.memoryTypeIndex = 0; }
        VkDeviceMemory bad_mem = VK_NULL_HANDLE;
        VkResult r = vkAllocateMemory(dev, &bad, nullptr, &bad_mem);
        std::printf("NEG_ALLOC iter=%u mode=%d result=%d(%s) fd=%d\n", iteration, mode, r, res_name(r), test_fd);
        CHECK(r == VK_ERROR_INVALID_EXTERNAL_HANDLE && bad_mem == VK_NULL_HANDLE, "negative allocate accepted");
        if (mode != 2) { CHECK(fcntl(test_fd, F_GETFD) >= 0, "failure consumed fd"); close(test_fd); }
        ded.pNext = nullptr;
    }

    ded.pNext = &import; ai.pNext = &ded; import.fd = fd;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VKCHECK(r_ == VK_SUCCESS, vkAllocateMemory(dev, &ai, nullptr, &memory));
    errno = 0;
    CHECK(fcntl(fd, F_GETFD) < 0 && errno == EBADF, "successful import did not consume original fd");
    std::printf("FD_SUCCESS iter=%u original_fd_consumed=1\n", iteration);
    if (!g_positive_only) {
        void *external_mapping = nullptr;
        VkResult map_result = vkMapMemory(dev, memory, 0, 4, 0, &external_mapping);
        std::printf("NEG_MAP iter=%u result=%d(%s)\n", iteration, map_result, res_name(map_result));
        CHECK(map_result == VK_ERROR_MEMORY_MAP_FAILED, "device-local-only external memory mapped");
    }

    VkBindImageMemoryInfo bind{VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO};
    bind.image = image; bind.memory = memory; bind.memoryOffset = 0;
    VKCHECK(r_ == VK_SUCCESS, api.BindImageMemory2KHR(dev, 1, &bind));

    VkBufferCreateInfo bci{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bci.size = BYTES; bci.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT; bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkBuffer buffer = VK_NULL_HANDLE; VKCHECK(r_ == VK_SUCCESS, vkCreateBuffer(dev, &bci, nullptr, &buffer));
    VkMemoryRequirements breq{}; vkGetBufferMemoryRequirements(dev, buffer, &breq);
    VkMemoryAllocateInfo bai{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    bai.allocationSize = breq.size; bai.memoryTypeIndex = 1;
    VkDeviceMemory buffer_memory = VK_NULL_HANDLE;
    VKCHECK(r_ == VK_SUCCESS, vkAllocateMemory(dev, &bai, nullptr, &buffer_memory));
    VKCHECK(r_ == VK_SUCCESS, vkBindBufferMemory(dev, buffer, buffer_memory, 0));

    VkCommandPoolCreateInfo pci{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO}; pci.queueFamilyIndex = 0;
    VkCommandPool pool = VK_NULL_HANDLE; VKCHECK(r_ == VK_SUCCESS, vkCreateCommandPool(dev, &pci, nullptr, &pool));
    CHECK(api.TrimCommandPoolKHR != nullptr, "vkTrimCommandPoolKHR missing");
    api.TrimCommandPoolKHR(dev, pool, 0);
    std::printf("TRIM_KHR iter=%u called=1\n", iteration);
    VkCommandBufferAllocateInfo cbai{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    cbai.commandPool = pool; cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cbai.commandBufferCount = 1;
    VkCommandBuffer cb = VK_NULL_HANDLE; VKCHECK(r_ == VK_SUCCESS, vkAllocateCommandBuffers(dev, &cbai, &cb));
    VkCommandBufferBeginInfo begin{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    VKCHECK(r_ == VK_SUCCESS, vkBeginCommandBuffer(cb, &begin));
    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.srcAccessMask = 0; barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image; barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    VkBufferImageCopy region{};
    region.bufferOffset = 0; region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    region.imageExtent = {W, H, 1};
    vkCmdCopyImageToBuffer(cb, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, buffer, 1, &region);
    VKCHECK(r_ == VK_SUCCESS, vkEndCommandBuffer(cb));
    VkFenceCreateInfo fci{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO}; VkFence fence = VK_NULL_HANDLE;
    VKCHECK(r_ == VK_SUCCESS, vkCreateFence(dev, &fci, nullptr, &fence));
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO}; submit.commandBufferCount = 1; submit.pCommandBuffers = &cb;
    VKCHECK(r_ == VK_SUCCESS, vkQueueSubmit(queue, 1, &submit, fence));
    VKCHECK(r_ == VK_SUCCESS, vkWaitForFences(dev, 1, &fence, VK_TRUE, 5000000000ull));
    void *mapped = nullptr; VKCHECK(r_ == VK_SUCCESS, vkMapMemory(dev, buffer_memory, 0, BYTES, 0, &mapped));
    std::memcpy(readback, mapped, BYTES); vkUnmapMemory(dev, buffer_memory);
    if (std::memcmp(expected, readback, BYTES) != 0) {
        for (unsigned i = 0; i < BYTES; ++i) if (expected[i] != readback[i]) {
            std::printf("PIXEL_MISMATCH iter=%u byte=%u expected=%02x actual=%02x\n", iteration, i, expected[i], readback[i]);
            break;
        }
    }
    CHECK(std::memcmp(expected, readback, BYTES) == 0, "pixel comparison");
    std::printf("PIXELS iter=%u exact=1 bytes=%u format=%d offset=%llu\n", iteration, BYTES, format,
                (unsigned long long)plane_offset);

    vkDestroyFence(dev, fence, nullptr); vkDestroyCommandPool(dev, pool, nullptr);
    vkDestroyBuffer(dev, buffer, nullptr); vkFreeMemory(dev, buffer_memory, nullptr);
    vkDestroyImage(dev, image, nullptr); vkFreeMemory(dev, memory, nullptr);
    gbm_bo_destroy(bo);
    std::printf("TEARDOWN iter=%u complete=1\n", iteration);
}

int main(int argc, char **argv) {
    try {
        g_positive_only = argc == 3 && std::strcmp(argv[2], "--positive-only") == 0;
        CHECK(argc == 2 || g_positive_only, "usage: client candidate-icd.json [--positive-only]");
        CHECK(setenv("VK_ICD_FILENAMES", argv[1], 1) == 0, "setenv VK_ICD_FILENAMES");
        std::printf("CLIENT candidate_icd=%s positive_only=%d\n", argv[1], g_positive_only);
        VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO}; app.apiVersion = VK_API_VERSION_1_0;
        const char *instance_exts[] = {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
                                       VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME};
        VkInstanceCreateInfo ici{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
        ici.pApplicationInfo = &app; ici.enabledExtensionCount = 2; ici.ppEnabledExtensionNames = instance_exts;
        VkInstance instance = VK_NULL_HANDLE; VKCHECK(r_ == VK_SUCCESS, vkCreateInstance(&ici, nullptr, &instance));
        uint32_t physical_count = 1; VkPhysicalDevice physical = VK_NULL_HANDLE;
        VKCHECK(r_ == VK_SUCCESS, vkEnumeratePhysicalDevices(instance, &physical_count, &physical));
        VkPhysicalDeviceProperties properties{}; vkGetPhysicalDeviceProperties(physical, &properties);
        std::printf("VULKAN_DEVICE name=%s api=%u.%u.%u vendor=0x%x device=0x%x\n", properties.deviceName,
                    VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion),
                    VK_VERSION_PATCH(properties.apiVersion), properties.vendorID, properties.deviceID);
        CHECK(std::strstr(properties.deviceName, "LoongGPU") && !std::strstr(properties.deviceName, "llvmpipe"), "not real vkgl LG110");
        VkPhysicalDeviceMemoryProperties memory_properties{};
        vkGetPhysicalDeviceMemoryProperties(physical, &memory_properties);
        std::printf("MEMORY_TYPES count=%u external_type2_flags=0x%x heap=%u\n", memory_properties.memoryTypeCount,
                    memory_properties.memoryTypes[2].propertyFlags, memory_properties.memoryTypes[2].heapIndex);
        CHECK(memory_properties.memoryTypeCount == 3 &&
              memory_properties.memoryTypes[2].propertyFlags == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
              "external memory type is not device-local-only");
        uint32_t ext_count = 0; VKCHECK(r_ == VK_SUCCESS, vkEnumerateDeviceExtensionProperties(physical, nullptr, &ext_count, nullptr));
        std::vector<VkExtensionProperties> available(ext_count);
        VKCHECK(r_ == VK_SUCCESS, vkEnumerateDeviceExtensionProperties(physical, nullptr, &ext_count, available.data()));
        for (const auto &e : available) std::printf("DEVICE_EXTENSION %s %u\n", e.extensionName, e.specVersion);
        std::vector<const char *> device_exts = {
            VK_KHR_MAINTENANCE_1_EXTENSION_NAME, VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
            VK_KHR_BIND_MEMORY_2_EXTENSION_NAME, VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
            VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME, VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
            VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME, VK_EXT_EXTERNAL_MEMORY_DMA_BUF_EXTENSION_NAME,
            VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME, VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME};
        VkDeviceCreateInfo dci{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
        dci.enabledExtensionCount = uint32_t(device_exts.size()); dci.ppEnabledExtensionNames = device_exts.data();
        VkDeviceQueueCreateInfo qci{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
        float priority = 1.0f; qci.queueFamilyIndex = 0; qci.queueCount = 1; qci.pQueuePriorities = &priority;
        dci.queueCreateInfoCount = 1; dci.pQueueCreateInfos = &qci;
        VkDevice dev = VK_NULL_HANDLE; VKCHECK(r_ == VK_SUCCESS, vkCreateDevice(physical, &dci, nullptr, &dev));
        VkQueue queue = VK_NULL_HANDLE; vkGetDeviceQueue(dev, 0, 0, &queue);
        Api api{};
        api.GetPhysicalDeviceFeatures2KHR = (PFN_vkGetPhysicalDeviceFeatures2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFeatures2KHR");
        api.GetPhysicalDeviceFormatProperties2KHR = (PFN_vkGetPhysicalDeviceFormatProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceFormatProperties2KHR");
        api.GetPhysicalDeviceImageFormatProperties2KHR = (PFN_vkGetPhysicalDeviceImageFormatProperties2KHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceImageFormatProperties2KHR");
        api.GetPhysicalDeviceExternalBufferPropertiesKHR = (PFN_vkGetPhysicalDeviceExternalBufferPropertiesKHR)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceExternalBufferPropertiesKHR");
        api.GetImageMemoryRequirements2KHR = (PFN_vkGetImageMemoryRequirements2KHR)vkGetDeviceProcAddr(dev, "vkGetImageMemoryRequirements2KHR");
        api.GetBufferMemoryRequirements2KHR = (PFN_vkGetBufferMemoryRequirements2KHR)vkGetDeviceProcAddr(dev, "vkGetBufferMemoryRequirements2KHR");
        api.GetImageSparseMemoryRequirements2KHR = (PFN_vkGetImageSparseMemoryRequirements2KHR)vkGetDeviceProcAddr(dev, "vkGetImageSparseMemoryRequirements2KHR");
        api.GetMemoryFdPropertiesKHR = (PFN_vkGetMemoryFdPropertiesKHR)vkGetDeviceProcAddr(dev, "vkGetMemoryFdPropertiesKHR");
        api.GetMemoryFdKHR = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(dev, "vkGetMemoryFdKHR");
        api.GetImageDrmFormatModifierPropertiesEXT = (PFN_vkGetImageDrmFormatModifierPropertiesEXT)vkGetDeviceProcAddr(dev, "vkGetImageDrmFormatModifierPropertiesEXT");
        api.BindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)vkGetDeviceProcAddr(dev, "vkBindImageMemory2KHR");
        api.BindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)vkGetDeviceProcAddr(dev, "vkBindBufferMemory2KHR");
        api.CreateSamplerYcbcrConversionKHR = (PFN_vkCreateSamplerYcbcrConversionKHR)vkGetDeviceProcAddr(dev, "vkCreateSamplerYcbcrConversionKHR");
        api.DestroySamplerYcbcrConversionKHR = (PFN_vkDestroySamplerYcbcrConversionKHR)vkGetDeviceProcAddr(dev, "vkDestroySamplerYcbcrConversionKHR");
        api.TrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)vkGetDeviceProcAddr(dev, "vkTrimCommandPoolKHR");
#define SHOW_ENTRY(p) std::printf("ENTRY %s present=%d\n", #p, api.p != nullptr)
        SHOW_ENTRY(GetPhysicalDeviceFeatures2KHR); SHOW_ENTRY(GetPhysicalDeviceFormatProperties2KHR);
       
        SHOW_ENTRY(GetPhysicalDeviceExternalBufferPropertiesKHR); SHOW_ENTRY(GetImageMemoryRequirements2KHR);
        SHOW_ENTRY(GetBufferMemoryRequirements2KHR); SHOW_ENTRY(GetImageSparseMemoryRequirements2KHR);
        SHOW_ENTRY(GetMemoryFdPropertiesKHR); SHOW_ENTRY(GetMemoryFdKHR); SHOW_ENTRY(GetImageDrmFormatModifierPropertiesEXT);
        SHOW_ENTRY(BindImageMemory2KHR); SHOW_ENTRY(BindBufferMemory2KHR); SHOW_ENTRY(CreateSamplerYcbcrConversionKHR);
        SHOW_ENTRY(DestroySamplerYcbcrConversionKHR); SHOW_ENTRY(TrimCommandPoolKHR);
#undef SHOW_ENTRY
        CHECK(api.GetPhysicalDeviceFeatures2KHR && api.GetPhysicalDeviceFormatProperties2KHR &&
              api.GetPhysicalDeviceImageFormatProperties2KHR &&
              api.GetPhysicalDeviceExternalBufferPropertiesKHR && api.GetImageMemoryRequirements2KHR &&
              api.GetBufferMemoryRequirements2KHR && api.GetImageSparseMemoryRequirements2KHR &&
              api.GetMemoryFdPropertiesKHR && api.GetMemoryFdKHR && api.GetImageDrmFormatModifierPropertiesEXT &&
              api.BindImageMemory2KHR && api.BindBufferMemory2KHR && api.CreateSamplerYcbcrConversionKHR &&
              api.DestroySamplerYcbcrConversionKHR, "required entry point missing");

        VkPhysicalDeviceSamplerYcbcrConversionFeatures ycbcr{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES};
        VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2}; features.pNext = &ycbcr;
        api.GetPhysicalDeviceFeatures2KHR(physical, &features);
        std::printf("YCBCR feature=%d (required 0)\n", ycbcr.samplerYcbcrConversion);
        CHECK(ycbcr.samplerYcbcrConversion == VK_FALSE, "Ycbcr feature advertised");
        if (!g_positive_only) {
            VkSamplerYcbcrConversionCreateInfo yci{VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO};
            yci.format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
            VkSamplerYcbcrConversion conversion = VK_NULL_HANDLE;
            VkResult yr = api.CreateSamplerYcbcrConversionKHR(dev, &yci, nullptr, &conversion);
            std::printf("NEG_YCBCR result=%d(%s) conversion=%p\n", yr, res_name(yr), (void *)conversion);
            CHECK(yr == VK_ERROR_FORMAT_NOT_SUPPORTED && conversion == VK_NULL_HANDLE, "Ycbcr conversion accepted");
        }

        for (VkFormat format : {VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_G8_B8R8_2PLANE_420_UNORM}) {
            VkDrmFormatModifierPropertiesEXT prop{};
            VkDrmFormatModifierPropertiesListEXT list{VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT};
            list.drmFormatModifierCount = 1; list.pDrmFormatModifierProperties = &prop;
            VkFormatProperties2 fp{VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2}; fp.pNext = &list;
            api.GetPhysicalDeviceFormatProperties2KHR(physical, format, &fp);
            std::printf("FORMAT_QUERY format=%d modifiers=%u optimal=0x%x linear=0x%x\n", format,
                        list.drmFormatModifierCount, fp.formatProperties.optimalTilingFeatures,
                        fp.formatProperties.linearTilingFeatures);
            if (format == VK_FORMAT_B8G8R8A8_UNORM) {
                CHECK(list.drmFormatModifierCount == 1 && prop.drmFormatModifier == 0 &&
                      prop.drmFormatModifierPlaneCount == 1 &&
                      (prop.drmFormatModifierTilingFeatures & (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT)) ==
                      (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_TRANSFER_SRC_BIT), "BGRA modifier query");
            } else {
                CHECK(list.drmFormatModifierCount == 0 && fp.formatProperties.optimalTilingFeatures == 0 &&
                      fp.formatProperties.linearTilingFeatures == 0, "NV12 advertised");
            }
        }

        VkPhysicalDeviceExternalImageFormatInfo ext_info{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO};
        ext_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
        VkPhysicalDeviceImageDrmFormatModifierInfoEXT modifier_info{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT};
        modifier_info.pNext = &ext_info; modifier_info.drmFormatModifier = 0; modifier_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        VkPhysicalDeviceImageFormatInfo2 query_info{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2};
        query_info.pNext = &modifier_info; query_info.format = VK_FORMAT_B8G8R8A8_UNORM;
        query_info.type = VK_IMAGE_TYPE_2D; query_info.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
        query_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkExternalImageFormatProperties external_out{VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES};
        VkImageFormatProperties2 query_out{VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2}; query_out.pNext = &external_out;
        VKCHECK(r_ == VK_SUCCESS, api.GetPhysicalDeviceImageFormatProperties2KHR(physical, &query_info, &query_out));
        CHECK((external_out.externalMemoryProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT),
              "image format pNext query");
        modifier_info.pNext = nullptr;
        VkImageFormatProperties2 modifier_only_out{VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2};
        VKCHECK(r_ == VK_SUCCESS, api.GetPhysicalDeviceImageFormatProperties2KHR(physical, &query_info, &modifier_only_out));
        std::printf("MODIFIER_ONLY_QUERY result=0 maxExtent=%ux%u\n",
                    modifier_only_out.imageFormatProperties.maxExtent.width,
                    modifier_only_out.imageFormatProperties.maxExtent.height);
        modifier_info.pNext = &ext_info;
        if (!g_positive_only) {
            modifier_info.drmFormatModifier = 1;
            VkResult qr = api.GetPhysicalDeviceImageFormatProperties2KHR(physical, &query_info, &query_out);
            std::printf("NEG_QUERY modifier_1 result=%d(%s)\n", qr, res_name(qr));
            CHECK(qr == VK_ERROR_FORMAT_NOT_SUPPORTED, "modifier query accepted");
            modifier_info.drmFormatModifier = 0;
            ext_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT;
            qr = api.GetPhysicalDeviceImageFormatProperties2KHR(physical, &query_info, &query_out);
            std::printf("NEG_QUERY opaque_fd result=%d(%s)\n", qr, res_name(qr));
            CHECK(qr == VK_ERROR_FORMAT_NOT_SUPPORTED, "opaque handle query accepted");
            ext_info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
        }

        int drm_fd = open("/dev/dri/renderD128", O_RDWR | O_CLOEXEC);
        CHECK(drm_fd >= 0, "open renderD128");
        gbm_device *gbm = gbm_create_device(drm_fd); CHECK(gbm, "gbm_create_device");
        if (!g_positive_only) negative_create_tests(dev, VK_FORMAT_B8G8R8A8_UNORM);
        run_positive(dev, queue, api, gbm, VK_FORMAT_B8G8R8A8_UNORM, 0, 0);
        run_positive(dev, queue, api, gbm, VK_FORMAT_R8G8B8A8_UNORM, 0, 1);
        run_positive(dev, queue, api, gbm, VK_FORMAT_B8G8R8A8_UNORM, 64, 2);
        run_positive(dev, queue, api, gbm, VK_FORMAT_R8G8B8A8_UNORM, 64, 3);
        run_positive(dev, queue, api, gbm, VK_FORMAT_R8G8B8A8_UNORM, 256, 4);
        if (!g_positive_only) {
            VkMemoryGetFdInfoKHR get_fd{VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR};
            get_fd.memory = VK_NULL_HANDLE; get_fd.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT;
            int export_fd = 123;
            VkResult export_result = api.GetMemoryFdKHR(dev, &get_fd, &export_fd);
            std::printf("NEG_EXPORT result=%d(%s) fd=%d\n", export_result, res_name(export_result), export_fd);
            CHECK(export_result == VK_ERROR_FEATURE_NOT_PRESENT && export_fd == -1, "export accepted");
        }
        gbm_device_destroy(gbm); close(drm_fd); vkDestroyDevice(dev, nullptr); vkDestroyInstance(instance, nullptr);
        if (g_positive_only)
            std::printf("PASS external_memory positives=5 exact_bytes=%u positive_only=1 intentional_negatives=0\n", BYTES * 5);
        else
            std::printf("PASS external_memory positives=5 exact_bytes=%u negative_create=10 negative_alloc=20 negative_queries=2 negative_map=5 negative_layout=5 trim_khr=5 ycbcr=0\n", BYTES * 5);
        return 0;
    } catch (const std::exception &) {
        std::printf("RUN_RESULT=FAIL\n");
        return 1;
    }
}
