#include <_types/_uint64_t.h>
#include <fstream>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
#include <glm/vec2.hpp>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    spdlog::info("VkResult: {}", static_cast<int>(err));                       \
    if (err) {                                                                 \
      spdlog::error("Detected Vulkan error: {}", static_cast<int>(err));       \
      throw std::runtime_error("Got a runtime_error");                         \
    }                                                                          \
  } while (0);

void initGLFW() {
  // Initialize GLFW
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }
  // Tell GLFW not to create an OpenGL context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

GLFWwindow *createGLFWwindow() {
  GLFWwindow *window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
  if (!window) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }
  return window;
}

std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  return buffer;
}

VkShaderModule loadShaderModule(const VkDevice &device, const char *path) {
  spdlog::info("Loading shader module {}", path);
  VkShaderModule shaderModule;
  std::vector<char> code = readFile(path);
  VkShaderModuleCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = nullptr,
      .flags = 0,
      .codeSize = code.size(),
      .pCode = reinterpret_cast<const uint32_t *>(code.data()),
  };
  spdlog::info("Creating shader module");
  VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule));
  return shaderModule;
}

void enumerateExtensions(const VkPhysicalDevice &physicalDevice) {
  // enumerate all extension properties
  uint32_t deviceExtensionCount;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                       &deviceExtensionCount, nullptr);
  std::vector<VkExtensionProperties> deviceExtensions(deviceExtensionCount);
  vkEnumerateDeviceExtensionProperties(
      physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data());

  spdlog::info("Device has {} extensions", deviceExtensionCount);
  for (const auto &extension : deviceExtensions) {
    spdlog::info("{}", extension.extensionName);
  }
}

VkInstance setupVulkanInstance() {
  VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "Planet",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "Planet Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_2,
  };
  uint32_t extensionCount;
  const char **reqExtensions =
      glfwGetRequiredInstanceExtensions(&extensionCount);
  spdlog::info("Required extensions count: {}", extensionCount);
  for (uint32_t i = 0; i < extensionCount; i++) {
    spdlog::info("{}", reqExtensions[i]);
  }

  // Create an array of extensions which include
  // VK_KHR_portability_enumeration and VK_MVK_MACOS_SURFACE_EXTENSION_NAME
  std::vector<const char *> extensions;
  for (uint32_t i = 0; i < extensionCount; i++) {
    extensions.emplace_back(reqExtensions[i]);
  }
  extensions.emplace_back("VK_KHR_portability_enumeration");

  spdlog::info("Using the following extensions");
  for (const auto &extension : extensions) {
    spdlog::info("{}", extension);
  }

  spdlog::info("Creating vk instance");

  // Enable validation layers
  // as a c++ std::array with the basic validation layer
  static constexpr std::array<const char *, 0> validationLayers = {
      // VK_LAYER_KHRONOS_validation seems to have a bug in dynamic rendering so
      // can't seem to enable it
      // "VK_LAYER_KHRONOS_validation",
      // api dump
      // "VK_LAYER_LUNARG_api_dump",
      // "VK_LAYER_LUNARG_parameter_validation",
      // "VK_LAYER_LUNARG_screenshot",
      // "VK_LAYER_LUNARG_core_validation",
      // "VK_LAYER_LUNARG_device_limits",
      // "VK_LAYER_LUNARG_object_tracker",
  };

  VkInstanceCreateInfo createInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = nullptr,
      .flags = VkInstanceCreateFlagBits::
          VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
      .pApplicationInfo = &appInfo,
      .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
      .ppEnabledLayerNames = validationLayers.data(),
      .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
      .ppEnabledExtensionNames = extensions.data(),
  };

  VkInstance instance;
  VK_CHECK(vkCreateInstance(&createInfo, nullptr, &instance));
  return instance;
}

VkPhysicalDevice findGPU(const VkInstance &instance) {
  // Find all GPU Devices
  uint32_t deviceCount = 0;
  spdlog::info("Enumerating devices...");
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  spdlog::info("Found {} devices", deviceCount);

  // Save devices to vector
  std::vector<VkPhysicalDevice> devices{deviceCount};
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  // Print all devices
  for (uint32_t i = 0; i < deviceCount; i++) {
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(devices[i], &deviceProperties);
    spdlog::info("Device {} has Vulkan version {}", i,
                 deviceProperties.apiVersion);
    spdlog::info("Device {} has driver version {}", i,
                 deviceProperties.driverVersion);
    spdlog::info("Device {} has vendor ID {}", i, deviceProperties.vendorID);
    spdlog::info("Device {} has device ID {}", i, deviceProperties.deviceID);
    spdlog::info("Device {} has device type {}", i,
                 static_cast<uint32_t>(deviceProperties.deviceType));
    spdlog::info("Device {} has device name {}", i,
                 deviceProperties.deviceName);
  }

  // just return 1st device (assume 1 GPU and use 1 GPU)
  return devices[0];
}

VkSurfaceKHR createVulkanSurface(const VkInstance &instance,
                                 GLFWwindow *const &window) {
  VkSurfaceKHR surface;
  VkResult createWindowResult =
      glfwCreateWindowSurface(instance, window, nullptr, &surface);
  if (createWindowResult != VK_SUCCESS) {
    spdlog::info("Failed to create Vulkan surface");
    spdlog::info("Result: {}", static_cast<int>(createWindowResult));
    vkDestroyInstance(instance, nullptr);
    glfwTerminate();
    throw std::runtime_error("Failed to create Vulkan surface");
  }
  return surface;
}

uint32_t getVulkanGraphicsQueueIndex(const VkPhysicalDevice &physicalDevice,
                                     const VkSurfaceKHR &surface) {
  int32_t graphicsQueueIndex = -1;

  // https://github.com/KhronosGroup/Vulkan-Samples/blob/cc7b29696011e7499379695947b9e634ed61ea10/samples/api/hello_triangle/hello_triangle.cpp#L293
  // Just use GPU 0 for now

  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);

  spdlog::info("Found {} queue families", queueFamilyCount);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queueFamilies.data());

  // Print debug info for all queue families
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    spdlog::info("Queue family {} has {} queues", i,
                 queueFamilies[i].queueCount);
    spdlog::info("Queue family {} supports graphics: {} ", i,
                 queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT);
    spdlog::info("Queue family {} supports compute: {} ", i,
                 queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT);
    spdlog::info("Queue family {} supports transfer: {} ", i,
                 queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT);
    spdlog::info("Queue family {} supports sparse binding: {} ", i,
                 queueFamilies[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT);
    spdlog::info("Queue family {} supports protected: {} ", i,
                 queueFamilies[i].queueFlags & VK_QUEUE_PROTECTED_BIT);

    VkBool32 supportsPresent;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                         &supportsPresent);
    spdlog::info("Queue family {} supports present: {} ", i, supportsPresent);

    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
        supportsPresent) {
      graphicsQueueIndex = i;
    }
  }

  if (graphicsQueueIndex == -1) {
    throw std::runtime_error("Failed to find graphics queue");
  }
  return static_cast<uint32_t>(graphicsQueueIndex);
}

VkDevice createVulkanLogicalDevice(const VkPhysicalDevice &physicalDevice,
                                   const uint32_t &graphicsQueueIndex) {
  float queuePriority = 1.0f;
  // Create one queue
  VkDeviceQueueCreateInfo queueInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = static_cast<uint32_t>(graphicsQueueIndex),
      .queueCount = 1,
      .pQueuePriorities = &queuePriority,
  };

  std::array<const char *const, 3> requiredExtensions = {
      "VK_KHR_swapchain", "VK_KHR_portability_subset",
      "VK_KHR_dynamic_rendering"};

  // Create a logical device
  spdlog::info("Create a logical device...");
  VkDevice device;

  VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamicRenderingFeatures{
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR,
      .dynamicRendering = VK_TRUE,
  };

  VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &dynamicRenderingFeatures,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queueInfo,
      .enabledExtensionCount = requiredExtensions.size(),
      .ppEnabledExtensionNames = requiredExtensions.data(),
  };

  VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
  spdlog::info("Created logical device");

  // auto &requested_dynamic_rendering            =
  // gpu.request_extension_features<VkPhysicalDeviceDynamicRenderingFeaturesKHR>(VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR);
  // requested_dynamic_rendering.dynamicRendering = VK_TRUE;

  return device;
}

void logSurfaceCapabilities(
    const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  // Print the surface capabilities
  spdlog::info("Surface capabilities:");
  spdlog::info("minImageCount: {}", surfaceCapabilities.minImageCount);
  spdlog::info("maxImageCount: {}", surfaceCapabilities.maxImageCount);
  spdlog::info("currentExtent: {}x{}", surfaceCapabilities.currentExtent.width,
               surfaceCapabilities.currentExtent.height);
  spdlog::info("minImageExtent: {}x{}",
               surfaceCapabilities.minImageExtent.width,
               surfaceCapabilities.minImageExtent.height);
  spdlog::info("maxImageExtent: {}x{}",
               surfaceCapabilities.maxImageExtent.width,
               surfaceCapabilities.maxImageExtent.height);
  spdlog::info("maxImageArrayLayers: {}",
               surfaceCapabilities.maxImageArrayLayers);
  spdlog::info("supportedTransforms: {}",
               surfaceCapabilities.supportedTransforms);
  spdlog::info("currentTransform: {}",
               static_cast<uint8_t>(surfaceCapabilities.currentTransform));
  spdlog::info("supportedCompositeAlpha: {}",
               surfaceCapabilities.supportedCompositeAlpha);
  spdlog::info("supportedUsageFlags: {}",
               surfaceCapabilities.supportedUsageFlags);
}

void logSurfaceFormats(const std::vector<VkSurfaceFormatKHR> &surfaceFormats) {
  spdlog::info("List all surface formats");
  for (auto &surfaceFormat : surfaceFormats) {
    spdlog::info("Surface format: {}", static_cast<int>(surfaceFormat.format));
    spdlog::info("Color space: {}", static_cast<int>(surfaceFormat.colorSpace));
  }
}
VkSurfaceCapabilitiesKHR
getSurfaceCapabilities(const VkPhysicalDevice &physicalDevice,
                       const VkSurfaceKHR &surface) {
  // Surface Capabilities
  spdlog::info("Get surface capabilities");
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface,
                                                     &surfaceCapabilities));

  logSurfaceCapabilities(surfaceCapabilities);
  return surfaceCapabilities;
}
VkSurfaceFormatKHR selectSwapchainFormat(const VkPhysicalDevice &physicalDevice,
                                         const VkSurfaceKHR &surface) {
  // Get surface formats
  uint32_t surfaceFormatCount;
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface,
                                                &surfaceFormatCount, nullptr));
  spdlog::info("Surface format count: {}", surfaceFormatCount);

  std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(
      physicalDevice, surface, &surfaceFormatCount, surfaceFormats.data()));

  // logSurfaceFormats(surfaceFormats);

  // https://github.com/KhronosGroup/Vulkan-Samples/blob/cc7b29696011e7499379695947b9e634ed61ea10/samples/api/hello_triangle/hello_triangle.cpp#L443
  // Fallback format
  VkSurfaceFormatKHR surfaceFormat = surfaceFormats[0];
  auto preferredFormatList =
      std::vector<VkFormat>{VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB,
                            VK_FORMAT_A8B8G8R8_SRGB_PACK32};

  for (auto &candidate : surfaceFormats) {
    if (std::find(preferredFormatList.begin(), preferredFormatList.end(),
                  candidate.format) != preferredFormatList.end()) {
      surfaceFormat = candidate;
      break;
    }
  }

  return surfaceFormat;
}

VkSwapchainKHR
createSwapchain(const VkDevice &device, const VkSurfaceKHR &surface,
                const VkSurfaceCapabilitiesKHR &surfaceCapabilities,
                const VkSurfaceFormatKHR &surfaceFormat) {
  VkExtent2D swapchainSize;
  if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) {
    // TODO: Get swapchain_size.width from the window
    throw std::runtime_error(
        "Untested code... "
        "surfaceCapabilities.currentExtent.width == 0xFFFFFFFF");
  } else {
    spdlog::info("Surface capabilities current extent width is not 0xFFFFFFFF");
    swapchainSize = surfaceCapabilities.currentExtent;
    spdlog::info("Swapchain size: {}x{}", swapchainSize.width,
                 swapchainSize.height);
    // Tested on mac with molten vk and got sample value 1600x1200
  }

  // Determine the number of VkImage's to use in the swapchain.
  // Ideally, we desire to own 1 image at a time, the rest of the images can
  // either be rendered to and/or being queued up for display.
  uint32_t desiredSwapchainImages = surfaceCapabilities.minImageCount + 1;
  if ((surfaceCapabilities.maxImageCount > 0) &&
      (desiredSwapchainImages > surfaceCapabilities.maxImageCount)) {
    // Application must settle for fewer images than desired.
    desiredSwapchainImages = surfaceCapabilities.maxImageCount;
  }
  spdlog::info("Desired swapchain images: {}", desiredSwapchainImages);

  // Just set identity bit transform
  VkSurfaceTransformFlagBitsKHR preTransform =
      VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
  // FIFO must be supported by all implementations.
  VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;

  // Find a supported composite type.
  VkCompositeAlphaFlagBitsKHR composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  if (surfaceCapabilities.supportedCompositeAlpha &
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
    composite = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  } else if (surfaceCapabilities.supportedCompositeAlpha &
             VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) {
    composite = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  } else if (surfaceCapabilities.supportedCompositeAlpha &
             VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR) {
    composite = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
  } else if (surfaceCapabilities.supportedCompositeAlpha &
             VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
    composite = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
  }
  spdlog::info("Composite alpha: {}", static_cast<int>(composite));

  spdlog::info("Selected surface format");
  spdlog::info("Surface format: {}", static_cast<int>(surfaceFormat.format));
  spdlog::info("Color space: {}", static_cast<int>(surfaceFormat.colorSpace));

  // Create a swapchain
  spdlog::info("Create a swapchain");
  VkSwapchainCreateInfoKHR swapchainCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = nullptr,
      .surface = surface,
      .minImageCount = desiredSwapchainImages,
      .imageFormat = surfaceFormat.format,
      .imageColorSpace = surfaceFormat.colorSpace,
      .imageExtent.width = swapchainSize.width,
      .imageExtent.height = swapchainSize.height,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .preTransform = preTransform,
      .compositeAlpha = composite,
      .presentMode = swapchainPresentMode,
      .clipped = VK_TRUE,
      // temporary
      .oldSwapchain = VK_NULL_HANDLE,
  };

  VkSwapchainKHR swapchain;
  VK_CHECK(
      vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain));

  return swapchain;
}

std::vector<VkImage> getSwapchainImages(const VkDevice &logicalDevice,
                                        const VkSwapchainKHR &swapchain) {
  // Get swapchain images
  uint32_t swapchainImageCount;
  VK_CHECK(vkGetSwapchainImagesKHR(logicalDevice, swapchain,
                                   &swapchainImageCount, nullptr));
  spdlog::info("Swapchain image count: {}", swapchainImageCount);

  std::vector<VkImage> swapchainImages(swapchainImageCount);
  VK_CHECK(vkGetSwapchainImagesKHR(
      logicalDevice, swapchain, &swapchainImageCount, swapchainImages.data()));
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    spdlog::info("Swapchain image {}", i);
    // spdlog::info("Size of swpachain image variable is {}",
    //              sizeof(swapchainImages[i]));
    // Definitely wrong but just to see something in debug
    // spdlog::info("Swapchain image handle: {}",
    //              reinterpret_cast<uint64_t>(swapchainImages[i]));
  }
  return swapchainImages;
}

std::vector<VkImageView>
createSwapchainImageViews(const VkDevice &device,
                          const std::vector<VkImage> &swapchainImages,
                          const VkSurfaceFormatKHR &surfaceFormat) {
  // Create image views
  std::vector<VkImageView> swapchainImageViews(swapchainImages.size());
  for (uint32_t i = 0; i < swapchainImages.size(); i++) {
    VkImageViewCreateInfo imageViewCreateInfo{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .flags = 0,
        .image = swapchainImages[i],
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = surfaceFormat.format,
        .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,

    };
    imageViewCreateInfo.pNext = nullptr;

    VK_CHECK(vkCreateImageView(device, &imageViewCreateInfo, nullptr,
                               &swapchainImageViews[i]));
  }
  return swapchainImageViews;
}

VkCommandPool createCommandPool(const VkDevice &logicalDevice,
                                const uint32_t &graphicsQueueIndex) {
  spdlog::info("Create command pool");
  VkCommandPoolCreateInfo commandPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = graphicsQueueIndex,
  };
  VkCommandPool commandPool;

  VK_CHECK(vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, nullptr,
                               &commandPool));
  return commandPool;
}

std::vector<VkCommandBuffer>
createCommandBuffers(const VkDevice &logicalDevice,
                     const VkCommandPool &commandPool,
                     const uint32_t &commandBufferCount) {
  std::vector<VkCommandBuffer> commandBuffers(commandBufferCount);
  spdlog::info("Create command buffer");
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = nullptr,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = commandBufferCount,
  };
  VK_CHECK(vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo,
                                    commandBuffers.data()));
  return commandBuffers;
}

void renderScene(const VkImage &image, const VkImageView &imageView,
                 const VkSurfaceCapabilitiesKHR surfaceCapabilities,
                 const VkCommandBuffer &commandBuffer,
                 const VkPipeline &pipeline) {
  // spdlog::info("Check swapchain image view [0]");
  // spdlog::info("Swapchain image view handle: {}",
  //              reinterpret_cast<uint64_t>(swapchainImageViews[0]));

  VkRenderingAttachmentInfo colorAttachmentInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = imageView,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue.color = {1.0f, 1.0f, 1.0f, 1.0f}};

  spdlog::info("Current extent: {}x{}", surfaceCapabilities.currentExtent.width,
               surfaceCapabilities.currentExtent.height);

  VkRenderingInfo renderingInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
      .renderArea =
          {
              .offset = {0, 0},
              .extent = {surfaceCapabilities.currentExtent.width,
                         surfaceCapabilities.currentExtent.height},
          },
      .layerCount = 1,
      .colorAttachmentCount = 1,
      .pColorAttachments = &colorAttachmentInfo,
  };

  vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);

  VkImageMemoryBarrier imageMemoryBarrierDraw{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = 0,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0,
                       nullptr, 0, nullptr, 1, &imageMemoryBarrierDraw);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  vkCmdDraw(commandBuffer, 3, 1, 0, 0);

  VkImageMemoryBarrier imageMemoryBarrierPresent{
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange =
          {
              .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
              .baseMipLevel = 0,
              .levelCount = 1,
              .baseArrayLayer = 0,
              .layerCount = 1,
          },
  };

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &imageMemoryBarrierPresent);

  vkCmdEndRenderingKHR(commandBuffer);
}

VkSemaphore createSemaphore(const VkDevice &logicalDevice) {
  VkSemaphore semaphore;

  VkSemaphoreCreateInfo semaphoreCreateInfo{
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
  };

  VK_CHECK(vkCreateSemaphore(logicalDevice, &semaphoreCreateInfo, nullptr,
                             &semaphore));
  return semaphore;
};
std::vector<VkSemaphore> createSemaphores(const VkDevice &logicalDevice,
                                          const uint32_t &count) {
  std::vector<VkSemaphore> semaphores(count);
  for (auto &semaphore : semaphores) {
    semaphore = createSemaphore(logicalDevice);
  }
  return semaphores;
}

uint32_t acquireNextImage(const VkDevice &logicalDevice,
                          const VkSemaphore &imageAvailableSemaphore,
                          const VkSwapchainKHR &swapchain) {
  // hardcoded for now
  uint32_t imageIndex;
  VK_CHECK(vkAcquireNextImageKHR(logicalDevice, swapchain, UINT64_MAX,
                                 imageAvailableSemaphore, VK_NULL_HANDLE,
                                 &imageIndex));
  return imageIndex;
}

void queueSubmit(const VkCommandBuffer &commandBuffer,
                 const VkSwapchainKHR &swapchain, const VkQueue &queue,
                 const VkSemaphore &imageAvailableSemaphore,
                 const VkSemaphore &renderingFinishedSemaphore,
                 const VkFence &fence, const uint32_t &imageIndex) {

  std::array<VkPipelineStageFlags, 1> waitFlags = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &imageAvailableSemaphore,
      .pWaitDstStageMask = waitFlags.data(),
      .commandBufferCount = 1,
      .pCommandBuffers = &commandBuffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &renderingFinishedSemaphore,
  };

  VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, fence));

  VkPresentInfoKHR presentInfo{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &renderingFinishedSemaphore,
      .swapchainCount = 1,
      .pSwapchains = &swapchain,
      .pImageIndices = &imageIndex,
      .pResults = nullptr,
  };
  VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));
}

VkPipeline createPipeline(const VkDevice &logicalDevice,
                          const VkSurfaceCapabilitiesKHR &surfaceCapabilities) {
  spdlog::info("Create pipeline");

  // https://www.saschawillems.de/blog/2016/08/13/vulkan-tutorial-on-rendering-a-fullscreen-quad-without-buffers/
  VkPipelineVertexInputStateCreateInfo emptyVertexInputStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = nullptr,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = nullptr,
  };
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 0,
      .pSetLayouts = nullptr,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = nullptr,
  };
  VkPipelineLayout pipelineLayout;
  VK_CHECK(vkCreatePipelineLayout(logicalDevice, &pipelineLayoutCreateInfo,
                                  nullptr, &pipelineLayout));

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

  // Vertex shader stage of the pipeline
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module =
      loadShaderModule(logicalDevice, "shaders/fullscreenquad.spv");
  shaderStages[0].pName = "main";

  // Fragment shader stage of the pipeline
  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module =
      loadShaderModule(logicalDevice, "shaders/planet.spv");
  shaderStages[1].pName = "main";

  VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .cullMode = VK_CULL_MODE_FRONT_BIT,
      .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
      .lineWidth = 1.0f,
  };

  VkRect2D scissor{
      .offset = {0, 0},
      .extent = {surfaceCapabilities.currentExtent.width,
                 surfaceCapabilities.currentExtent.height},
  };

  VkViewport viewport{
      .x = 0.0f,
      .y = 0.0f,
      .width = static_cast<float>(surfaceCapabilities.currentExtent.width),
      .height = static_cast<float>(surfaceCapabilities.currentExtent.height),
      .minDepth = 0.0f,
      .maxDepth = 1.0f,
  };

  VkPipelineViewportStateCreateInfo viewportStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &viewport,
      .scissorCount = 1,
      .pScissors = &scissor,
  };

  VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT, // no multisampling
  };

  VkPipelineInputAssemblyStateCreateInfo assemblyStateCreateInfo{
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };

  VkPipelineColorBlendAttachmentState blendAttachment{};
  blendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  blendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo blend{
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
  blend.attachmentCount = 1;
  blend.pAttachments = &blendAttachment;

  // Disable all depth testing
  VkPipelineDepthStencilStateCreateInfo depthStencil{
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

  VkFormat defaultFormat = VK_FORMAT_R8G8B8A8_SRGB;

  // for dynamic rendering
  VkPipelineRenderingCreateInfoKHR dynamicPipelineCreate{
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
  dynamicPipelineCreate.pNext = VK_NULL_HANDLE;
  dynamicPipelineCreate.colorAttachmentCount = 1;
  dynamicPipelineCreate.pColorAttachmentFormats = &defaultFormat;
  dynamicPipelineCreate.depthAttachmentFormat = VK_FORMAT_D16_UNORM;

  VkGraphicsPipelineCreateInfo pipelineCreateInfo{
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = &dynamicPipelineCreate,
      .stageCount = static_cast<uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .pVertexInputState = &emptyVertexInputStateCreateInfo,
      .pInputAssemblyState = &assemblyStateCreateInfo,
      .pViewportState = &viewportStateCreateInfo,
      .pRasterizationState = &rasterizationStateCreateInfo,
      .pMultisampleState = &multisampleStateCreateInfo,
      .pDepthStencilState = &depthStencil,
      .pColorBlendState = &blend,
      .layout = pipelineLayout,
  };
  spdlog::info("Create the graphics pipeline");
  VkPipeline pipeline;
  VK_CHECK(vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1,
                                     &pipelineCreateInfo, nullptr, &pipeline));
  spdlog::info("Created the pipeline");
  return pipeline;
}

std::vector<VkFence> createFences(const VkDevice &logicalDevice,
                                  const uint32_t &count) {
  std::vector<VkFence> fences(count);
  for (uint32_t i = 0; i < count; i++) {
    VkFenceCreateInfo fenceCreateInfo{
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = static_cast<VkFenceCreateFlags>(
            (i == 0) ? VK_FENCE_CREATE_SIGNALED_BIT : 0),
    };
    VK_CHECK(
        vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &fences[i]));
  }
  return fences;
}

VkQueryPool createQueryPool(const VkDevice &logicalDevice,
                            const uint32_t &queryCount) {
  VkQueryPoolCreateInfo queryPoolCreateInfo{
      .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
      .queryType = VK_QUERY_TYPE_TIMESTAMP,
      .queryCount = queryCount,
  };
  VkQueryPool queryPool;
  VK_CHECK(vkCreateQueryPool(logicalDevice, &queryPoolCreateInfo, nullptr,
                             &queryPool));
  return queryPool;
}

int main() {
  spdlog::set_level(spdlog::level::err);
  initGLFW();
  GLFWwindow *window = createGLFWwindow();

  VkInstance instance = setupVulkanInstance();
  VkPhysicalDevice physicalDevice = findGPU(instance);
  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  enumerateExtensions(physicalDevice);

  VkSurfaceKHR surface = createVulkanSurface(instance, window);
  uint32_t graphicsQueueIndex =
      getVulkanGraphicsQueueIndex(physicalDevice, surface);
  VkDevice logicalDevice =
      createVulkanLogicalDevice(physicalDevice, graphicsQueueIndex);
  VkSurfaceCapabilitiesKHR surfaceCapabilities =
      getSurfaceCapabilities(physicalDevice, surface);
  VkSurfaceFormatKHR surfaceFormat =
      selectSwapchainFormat(physicalDevice, surface);
  VkSwapchainKHR swapchain = createSwapchain(
      logicalDevice, surface, surfaceCapabilities, surfaceFormat);
  std::vector<VkImage> swapchainImages =
      getSwapchainImages(logicalDevice, swapchain);
  std::vector<VkImageView> swapchainImageViews =
      createSwapchainImageViews(logicalDevice, swapchainImages, surfaceFormat);
  VkCommandPool commandPool =
      createCommandPool(logicalDevice, graphicsQueueIndex);
  VkPipeline pipeline = createPipeline(logicalDevice, surfaceCapabilities);
  // Create vkqueue
  VkQueue queue;
  vkGetDeviceQueue(logicalDevice, graphicsQueueIndex, 0, &queue);

  // Command buffers
  std::vector<VkCommandBuffer> commandBuffers =
      createCommandBuffers(logicalDevice, commandPool, swapchainImages.size());

  // Create fences and semaphores
  std::vector<VkFence> fences =
      createFences(logicalDevice, swapchainImages.size());
  std::vector<VkSemaphore> imageAvailableSemaphores =
      createSemaphores(logicalDevice, swapchainImages.size());
  std::vector<VkSemaphore> renderFinishedSemaphore =
      createSemaphores(logicalDevice, swapchainImages.size());

  auto queryPool = createQueryPool(logicalDevice, 2 * swapchainImages.size());

  uint32_t currentImage = 0;
  std::chrono::high_resolution_clock::time_point cpuStart, cpuEnd;
  while (!glfwWindowShouldClose(window)) {
    cpuStart = std::chrono::high_resolution_clock::now();

    glfwPollEvents();

    // Wait for the fence from the last frame before acquiring next image
    VK_CHECK(vkWaitForFences(logicalDevice, 1, &fences[currentImage], VK_TRUE,
                             UINT64_MAX));
    VK_CHECK(vkResetFences(logicalDevice, 1, &fences[currentImage]));
    VK_CHECK(vkResetCommandBuffer(commandBuffers[currentImage], 0));

    uint32_t imageIndex = acquireNextImage(
        logicalDevice, imageAvailableSemaphores[currentImage], swapchain);

    spdlog::info("Image index: {}", imageIndex);

    VkCommandBufferBeginInfo commandBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
    };

    VK_CHECK(vkBeginCommandBuffer(commandBuffers[imageIndex],
                                  &commandBufferBeginInfo));
    // Start GPU Timestamp
    vkCmdResetQueryPool(commandBuffers[currentImage], queryPool,
                        currentImage * 2, 2);

    vkCmdWriteTimestamp(commandBuffers[imageIndex],
                        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool,
                        currentImage * 2);
    renderScene(swapchainImages[imageIndex], swapchainImageViews[imageIndex],
                surfaceCapabilities, commandBuffers[imageIndex], pipeline);

    vkCmdWriteTimestamp(commandBuffers[imageIndex],
                        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool,
                        currentImage * 2 + 1);
    VK_CHECK(vkEndCommandBuffer(commandBuffers[imageIndex]));
    queueSubmit(commandBuffers[imageIndex], swapchain, queue,
                imageAvailableSemaphores[currentImage],
                renderFinishedSemaphore[currentImage],
                fences[(imageIndex + 1) % fences.size()], imageIndex);

    cpuEnd = std::chrono::high_resolution_clock::now();

    // Get Gpu time
    uint64_t gpuBegin, gpuEnd = 0;
    uint64_t times[2];
    spdlog::info("Waiting for GPU");

    // bad?
    // VK_CHECK(vkDeviceWaitIdle(logicalDevice));

    VK_CHECK(vkGetQueryPoolResults(
        logicalDevice, queryPool, currentImage * 2, 2, sizeof(uint64_t) * 2,
        &times, sizeof(uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));

    double totalCpuTime =
        std::chrono::duration_cast<std::chrono::nanoseconds>(cpuEnd - cpuStart)
            .count() *
        1e-6;
    double totalGpuTime =
        (times[1] - times[0]) * deviceProperties.limits.timestampPeriod * 1e-6;
    //
    // std::string title = fmt::format(
    //     "CPU: {:d}ms  GPU B: {:.3f}ms GPU E: {:.3f}ms", totalCpuTime,
    //     times[0] * deviceProperties.limits.timestampPeriod * 1e-6,
    //     times[1] * deviceProperties.limits.timestampPeriod * 1e-6);
    std::string title =
        fmt::format("CPU: {:.3f}ms  GPU: {:.3f}ms", totalCpuTime, totalGpuTime);
    glfwSetWindowTitle(window, title.c_str());

    currentImage = (currentImage + 1) % swapchainImages.size();

    /*
     * DEBUG
      break;
    }
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    */
  }

  // Free command buffers
  vkFreeCommandBuffers(logicalDevice, commandPool, commandBuffers.size(),
                       commandBuffers.data());

  // Cleanup after the main loop
  for (auto &semaphore : imageAvailableSemaphores) {
    vkDestroySemaphore(logicalDevice, semaphore, nullptr);
  }
  for (auto &semaphore : renderFinishedSemaphore) {
    vkDestroySemaphore(logicalDevice, semaphore, nullptr);
  }
  for (auto &fence : fences) {
    vkDestroyFence(logicalDevice, fence, nullptr);
  }
  for (auto &imageView : swapchainImageViews) {
    vkDestroyImageView(logicalDevice, imageView, nullptr);
  }
  vkDestroySwapchainKHR(logicalDevice, swapchain, nullptr);
  vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
  vkDestroyPipeline(logicalDevice, pipeline, nullptr);
  vkDestroyDevice(logicalDevice, nullptr);
  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
