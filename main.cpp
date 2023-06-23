#include <_types/_uint64_t.h>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

constexpr uint32_t MAX_DEVICES = 16;
constexpr uint32_t MAX_DEVICE_EXTENSIONS = 200;

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    spdlog::info("Result: {}", static_cast<int>(err));                         \
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

VkInstance setupVulkanInstance() {
  VkApplicationInfo appInfo = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = nullptr,
      .pApplicationName = "Planet",
      .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
      .pEngineName = "Planet Engine",
      .engineVersion = VK_MAKE_VERSION(1, 0, 0),
      .apiVersion = VK_API_VERSION_1_3,
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
  const char *extensions[99]{};
  for (uint32_t i = 0; i < extensionCount; i++) {
    extensions[i] = reqExtensions[i];
  }
  extensions[extensionCount] = "VK_KHR_portability_enumeration";

  spdlog::info("Using the following extensions");
  for (uint32_t i = 0; i <= extensionCount; i++) {
    spdlog::info("Extension: {}", extensions[i]);
  }
  spdlog::info("");

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
      .enabledExtensionCount = extensionCount + 1,
      .ppEnabledExtensionNames = extensions,
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
  std::array<VkPhysicalDevice, MAX_DEVICES> devices;
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

VkDevice createVulkanLogicalDevice(const VkPhysicalDevice &physicalDevice,
                                   const VkSurfaceKHR &surface) {
  float queue_priority = 1.0f;
  int32_t graphicsQueueIndex = -1;

  // https://github.com/KhronosGroup/Vulkan-Samples/blob/cc7b29696011e7499379695947b9e634ed61ea10/samples/api/hello_triangle/hello_triangle.cpp#L293
  // Just use GPU 0 for now

  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           nullptr);

  spdlog::info("Found {} queue families", queueFamilyCount);

  std::vector<VkQueueFamilyProperties> queue_families(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                           queue_families.data());

  // Print debug info for all queue families
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    spdlog::info("Queue family {} has {} queues", i,
                 queue_families[i].queueCount);
    spdlog::info("Queue family {} supports graphics: {} ", i,
                 queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT);
    spdlog::info("Queue family {} supports compute: {} ", i,
                 queue_families[i].queueFlags & VK_QUEUE_COMPUTE_BIT);
    spdlog::info("Queue family {} supports transfer: {} ", i,
                 queue_families[i].queueFlags & VK_QUEUE_TRANSFER_BIT);
    spdlog::info("Queue family {} supports sparse binding: {} ", i,
                 queue_families[i].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT);
    spdlog::info("Queue family {} supports protected: {} ", i,
                 queue_families[i].queueFlags & VK_QUEUE_PROTECTED_BIT);

    VkBool32 supportsPresent;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                         &supportsPresent);
    spdlog::info("Queue family {} supports present: {} ", i, supportsPresent);

    if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
        supportsPresent) {
      graphicsQueueIndex = i;
    }
  }

  if (graphicsQueueIndex == -1) {
    throw std::runtime_error("Failed to find graphics queue");
  }

  // Create one queue
  VkDeviceQueueCreateInfo queue_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .queueFamilyIndex = static_cast<uint32_t>(graphicsQueueIndex),
      .queueCount = 1,
      .pQueuePriorities = &queue_priority,
  };

  uint32_t device_extension_count;
  vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                       &device_extension_count, nullptr);
  VkExtensionProperties device_extensions[MAX_DEVICE_EXTENSIONS];
  vkEnumerateDeviceExtensionProperties(
      physicalDevice, nullptr, &device_extension_count, device_extensions);

  const char *const requiredExtensions[2] = {"VK_KHR_swapchain",
                                             "VK_KHR_portability_subset"};

  // Create a logical device
  spdlog::info("Create a logical device...");
  VkDevice device;
  VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &queue_info,
      .enabledExtensionCount = 2,
      .ppEnabledExtensionNames = requiredExtensions,
  };

  VK_CHECK(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
  spdlog::info("Created logical device");
  return device;
}

int main() {
  initGLFW();
  GLFWwindow *window = createGLFWwindow();

  VkInstance instance = setupVulkanInstance();
  VkPhysicalDevice device = findGPU(instance);
  VkSurfaceKHR surface = createVulkanSurface(instance, window);
  createVulkanLogicalDevice(device, surface);

  // while (!glfwWindowShouldClose(window)) {
  //   glfwPollEvents();
  // }

  return 0;
}
