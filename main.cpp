#include <_types/_uint64_t.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>

#define VK_CHECK(x)                                                            \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      spdlog::error("Detected Vulkan error: {}", static_cast<int>(err));       \
      throw std::runtime_error("Got a runtime_error");                         \
    }                                                                          \
  } while (0);

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
  const char *extensions[30]{};
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
  const std::array<const char *, 1> validationLayers = {
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

void findGPUs(VkInstance &instance) {
  // Find all GPU Devices
  uint32_t deviceCount = 0;
  spdlog::info("Enumerating devices...");
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
  spdlog::info("Found {} devices", deviceCount);

  // Save devices to vector
  VkPhysicalDevice devices[10];
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices);

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
}

int main() {
  spdlog::info("Hello world");
  // Initialize GLFW
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }
  VkInstance instance = setupVulkanInstance();
  findGPUs(instance);
  return 0;
}
