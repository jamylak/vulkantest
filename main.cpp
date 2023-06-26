#include <_types/_uint64_t.h>
#include <stdexcept>
#include <stdint.h>
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
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
  static constexpr std::array<const char *, 1> validationLayers = {
      "VK_LAYER_KHRONOS_validation",
      // "VK_LAYER_LUNARG_core_validation",
      // "VK_LAYER_LUNARG_device_limits",
      // "VK_LAYER_LUNARG_object_tracker",
      // "VK_LAYER_LUNARG_parameter_validation",
      // "VK_LAYER_LUNARG_screenshot",
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

std::vector<VkImageView>
createSwapchainImageViews(const VkDevice &device,
                          const VkSwapchainKHR &swapchain,
                          const VkSurfaceFormatKHR &surfaceFormat) {
  // Get swapchain images
  uint32_t swapchainImageCount;
  VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount,
                                   nullptr));
  spdlog::info("Swapchain image count: {}", swapchainImageCount);

  std::vector<VkImage> swapchainImages(swapchainImageCount);
  VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount,
                                   swapchainImages.data()));
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
    spdlog::info("Swapchain image {}", i);
    // spdlog::info("Size of swpachain image variable is {}",
    //              sizeof(swapchainImages[i]));
    // Definitely wrong but just to see something in debug
    // spdlog::info("Swapchain image handle: {}",
    //              reinterpret_cast<uint64_t>(swapchainImages[i]));
  }

  // Create image views
  std::vector<VkImageView> swapchainImageViews(swapchainImageCount);
  for (uint32_t i = 0; i < swapchainImageCount; i++) {
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

int main() {
  initGLFW();
  GLFWwindow *window = createGLFWwindow();

  VkInstance instance = setupVulkanInstance();
  VkPhysicalDevice physicalDevice = findGPU(instance);
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
  std::vector<VkImageView> swapchainImageViews =
      createSwapchainImageViews(logicalDevice, swapchain, surfaceFormat);
  VkCommandPool commandPool =
      createCommandPool(logicalDevice, graphicsQueueIndex);

  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo commandBufferAllocateInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  VK_CHECK(vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo,
                                    &commandBuffer));

  spdlog::info("Check swapchain image view [0]");
  spdlog::info("Size of swpachain image view variable is {}",
               sizeof(swapchainImageViews[0]));
  spdlog::info("Swapchain image view handle: {}",
               reinterpret_cast<uint64_t>(swapchainImageViews[0]));

  VkRenderingAttachmentInfo colorAttachmentInfo{
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = swapchainImageViews[0],
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .clearValue.color = {1.0f, 1.0f, 0.0f, 1.0f}};

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

  VkCommandBufferBeginInfo commandBufferBeginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
  };

  VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

  vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);
  vkCmdEndRenderingKHR(commandBuffer);

  VK_CHECK(vkEndCommandBuffer(commandBuffer));

  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
  }

  return 0;
}
