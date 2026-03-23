#include "vulkan_rhi.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vulkan/vulkan_core.h>

#ifdef __ANDROID__
#include <android/log.h>
#include <android/native_window.h>
#include <vulkan/vulkan_android.h>
#else
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif
#include <spdlog/spdlog.h>

static VkInstance sInstance = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT sDebugCallback = VK_NULL_HANDLE;
static VkSurfaceKHR sSurface = VK_NULL_HANDLE;
static VkPhysicalDevice sGPU = VK_NULL_HANDLE;
static uint32_t sGraphicQueueFamily = 0;
static uint32_t sPresentQueueFamily = 0;
static VkDevice sDevice = VK_NULL_HANDLE;
static VkQueue sGraphicQueue = VK_NULL_HANDLE;
static VkQueue sPresentQueue = VK_NULL_HANDLE;
static VkSurfaceCapabilitiesKHR sSurfaceCaps = {};
static std::vector<VkSurfaceFormatKHR> sSurfaceFormats;
static std::vector<VkPresentModeKHR> sPresentModes;
static VkSwapchainKHR sSwapChain = VK_NULL_HANDLE;
static std::vector<VkImage> sSwapChainImages;
static std::vector<VkImageView> sSwapChainImageViews;
static std::vector<VkFramebuffer> sSwapChainFBOs;
static Texture *sDepthBuffer = nullptr;
static VkRenderPass sSwapChainRenderPass = VK_NULL_HANDLE;
static VkCommandPool sCommandPool = VK_NULL_HANDLE;
static VkSemaphore sReadyToRender = VK_NULL_HANDLE;
static VkSemaphore sReadyToPresent = VK_NULL_HANDLE;
static VkFence sInFlightFence = VK_NULL_HANDLE;
static uint32_t sCanvasWidth = 1280;
static uint32_t sCanvasHeight = 720;
static VkFormat sSwapChainFormat = VK_FORMAT_B8G8R8A8_UNORM;

static ShaderParameterDescription sUberShaderParams;

#ifdef __ANDROID__
static AAssetManager *sAssetManager = nullptr;

void SetAndroidAssetManager(AAssetManager *mgr) { sAssetManager = mgr; }
AAssetManager *GetAndroidAssetManager() { return sAssetManager; }
#endif

static VkPhysicalDeviceMemoryProperties sPhysicalMemProps = {};
static bool sPhysicalMemPropsCached = false;

static PFN_vkCreateDebugReportCallbackEXT fnCreateDebugReport = nullptr;
static PFN_vkDestroyDebugReportCallbackEXT fnDestroyDebugReport = nullptr;
static PFN_vkCmdBeginDebugUtilsLabelEXT fnBeginDebugLabel = nullptr;
static PFN_vkCmdEndDebugUtilsLabelEXT fnEndDebugLabel = nullptr;
static PFN_vkSetDebugUtilsObjectNameEXT fnSetObjectName = nullptr;

static void EnsurePhysicalMemProps() {
  if (!sPhysicalMemPropsCached) {
    vkGetPhysicalDeviceMemoryProperties(sGPU, &sPhysicalMemProps);
    sPhysicalMemPropsCached = true;
  }
}

static uint32_t FindMemoryType(uint32_t typeFilter,
                               VkMemoryPropertyFlags properties) {
  EnsurePhysicalMemProps();
  for (uint32_t i = 0; i < sPhysicalMemProps.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (sPhysicalMemProps.memoryTypes[i].propertyFlags & properties) ==
            properties) {
      return i;
    }
  }
  spdlog::error("Failed to find suitable memory type");
  return 0;
}

static std::vector<const char *> GetValidationLayers() {
  std::vector<const char *> layers;
  uint32_t layerCount = 0;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> available(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, available.data());

  for (const auto &layer : available) {
    if (std::strstr(layer.layerName, "validation") != nullptr) {
      layers.push_back("VK_LAYER_KHRONOS_validation");
      break;
    }
  }
  return layers;
}

static bool InitInstance() {
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Nano";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 2, 0);
  appInfo.pEngineName = "Nano";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 2, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

#ifdef __ANDROID__
  std::vector<const char *> extensions = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
  };
#else
  uint32_t glfwExtCount = 0;
  const char **glfwExts = glfwGetRequiredInstanceExtensions(&glfwExtCount);
  std::vector<const char *> extensions(glfwExts, glfwExts + glfwExtCount);
#endif

#ifndef NDEBUG
  extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

  auto validationLayers = GetValidationLayers();

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();
#ifndef NDEBUG
  createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
  createInfo.ppEnabledLayerNames = validationLayers.data();
#endif

  if (vkCreateInstance(&createInfo, nullptr, &sInstance) != VK_SUCCESS) {
    spdlog::error("Failed to create Vulkan instance");
    return false;
  }
  return true;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(
    VkDebugReportFlagsEXT flags,
    [[maybe_unused]] VkDebugReportObjectTypeEXT objectType,
    [[maybe_unused]] uint64_t object, [[maybe_unused]] size_t location,
    [[maybe_unused]] int32_t messageCode,
    [[maybe_unused]] const char *pLayerPrefix,
    [[maybe_unused]] const char *pMessage, [[maybe_unused]] void *pUserData) {
  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    spdlog::error("[Vulkan] {}", pMessage);
  } else {
    spdlog::warn("[Vulkan] {}", pMessage);
  }
  return VK_FALSE;
}

static bool InitDebugger() {
  VkDebugReportCallbackCreateInfoEXT info = {};
  info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
  info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
  info.pfnCallback = DebugReportCallback;
  return fnCreateDebugReport(sInstance, &info, nullptr, &sDebugCallback) ==
         VK_SUCCESS;
}

#ifdef __ANDROID__
static bool InitSurface(ANativeWindow *window) {
  VkAndroidSurfaceCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
  createInfo.window = window;
  return vkCreateAndroidSurfaceKHR(sInstance, &createInfo, nullptr, &sSurface) ==
         VK_SUCCESS;
}
#else
static bool InitSurface(GLFWwindow *window) {
  return glfwCreateWindowSurface(sInstance, window, nullptr, &sSurface) ==
         VK_SUCCESS;
}
#endif

static bool TryPickQueueFamilies(VkPhysicalDevice device,
                                 uint32_t *outGraphicIdx,
                                 uint32_t *outPresentIdx) {
  uint32_t queueCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueProps(queueCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueCount,
                                           queueProps.data());

  int graphicIdx = -1, presentIdx = -1;
  for (uint32_t j = 0; j < queueCount; j++) {
    if (queueProps[j].queueCount > 0 &&
        (queueProps[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
      graphicIdx = static_cast<int>(j);
    }
    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, j, sSurface, &presentSupport);
    if (queueProps[j].queueCount > 0 && presentSupport) {
      presentIdx = static_cast<int>(j);
    }
    if (graphicIdx >= 0 && presentIdx >= 0) {
      *outGraphicIdx = static_cast<uint32_t>(graphicIdx);
      *outPresentIdx = static_cast<uint32_t>(presentIdx);
      return true;
    }
  }
  return false;
}

static bool InitPhysicalDevice() {
  uint32_t count = 0;
  vkEnumeratePhysicalDevices(sInstance, &count, nullptr);
  if (count == 0) {
    spdlog::error("No Vulkan physical devices found");
    return false;
  }
  std::vector<VkPhysicalDevice> devices(count);
  vkEnumeratePhysicalDevices(sInstance, &count, devices.data());

  VkPhysicalDevice bestDevice = VK_NULL_HANDLE;
  uint32_t bestG = 0, bestP = 0;
  int bestScore = -1;

  for (auto &device : devices) {
    uint32_t g = 0, p = 0;
    if (!TryPickQueueFamilies(device, &g, &p)) {
      continue;
    }
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);
    int score = 0;
    switch (props.deviceType) {
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
      score = 100;
      break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
      score = 50;
      break;
    default:
      score = 10;
      break;
    }
    if (score > bestScore) {
      bestScore = score;
      bestDevice = device;
      bestG = g;
      bestP = p;
    }
  }

  if (bestDevice == VK_NULL_HANDLE) {
    spdlog::error("No suitable GPU found");
    return false;
  }

  sGPU = bestDevice;
  sGraphicQueueFamily = bestG;
  sPresentQueueFamily = bestP;
  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(bestDevice, &props);
  spdlog::info("GPU-{}: {}", props.deviceID, props.deviceName);
  return true;
}

static bool InitLogicDevice() {
  float priority = 1.0f;
  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

  VkDeviceQueueCreateInfo qci = {};
  qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  qci.queueFamilyIndex = sGraphicQueueFamily;
  qci.queueCount = 1;
  qci.pQueuePriorities = &priority;
  queueCreateInfos.push_back(qci);

  if (sGraphicQueueFamily != sPresentQueueFamily) {
    qci.queueFamilyIndex = sPresentQueueFamily;
    queueCreateInfos.push_back(qci);
  }

  const char *deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  VkPhysicalDeviceShaderAtomicInt64Features atomic64 = {};
  atomic64.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
  atomic64.shaderBufferInt64Atomics = VK_TRUE;

  VkPhysicalDeviceFeatures2 features2 = {};
  features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
  features2.features.shaderInt64 = VK_TRUE;
  features2.features.fragmentStoresAndAtomics = VK_TRUE;
#ifndef __ANDROID__
  features2.features.fillModeNonSolid = VK_TRUE;
#endif
  features2.pNext = &atomic64;

  VkDeviceCreateInfo deviceInfo = {};
  deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
  deviceInfo.enabledExtensionCount = 1;
  deviceInfo.ppEnabledExtensionNames = deviceExtensions;
  deviceInfo.pNext = &features2;

  if (vkCreateDevice(sGPU, &deviceInfo, nullptr, &sDevice) != VK_SUCCESS) {
    spdlog::error("Failed to create logical device");
    return false;
  }

  vkGetDeviceQueue(sDevice, sGraphicQueueFamily, 0, &sGraphicQueue);
  vkGetDeviceQueue(sDevice, sPresentQueueFamily, 0, &sPresentQueue);
  spdlog::info("Queue mode: {}", sPresentQueueFamily == sGraphicQueueFamily
                                     ? "Shared"
                                     : "Separate");
  return true;
}

static void InitSurfaceProperties() {
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(sGPU, sSurface, &sSurfaceCaps);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(sGPU, sSurface, &formatCount, nullptr);
  sSurfaceFormats.resize(formatCount);
  vkGetPhysicalDeviceSurfaceFormatsKHR(sGPU, sSurface, &formatCount,
                                       sSurfaceFormats.data());

  uint32_t modeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(sGPU, sSurface, &modeCount,
                                            nullptr);
  sPresentModes.resize(modeCount);
  vkGetPhysicalDeviceSurfacePresentModesKHR(sGPU, sSurface, &modeCount,
                                            sPresentModes.data());
}

static bool InitSwapchain() {
  if (sSurfaceFormats.empty()) {
    spdlog::error("No Vulkan surface formats available");
    return false;
  }

  VkSurfaceFormatKHR selectedFormat = sSurfaceFormats[0];
  for (auto &fmt : sSurfaceFormats) {
    if ((fmt.format == VK_FORMAT_B8G8R8A8_UNORM ||
         fmt.format == VK_FORMAT_R8G8B8A8_UNORM) &&
        fmt.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
      selectedFormat = fmt;
      break;
    }
  }
  sSwapChainFormat = selectedFormat.format;

  VkSwapchainCreateInfoKHR info = {};
  info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  info.surface = sSurface;
  info.minImageCount = std::max(2u, sSurfaceCaps.minImageCount);
  info.imageFormat = selectedFormat.format;
  info.imageColorSpace = selectedFormat.colorSpace;
  info.imageExtent = {sCanvasWidth, sCanvasHeight};
  info.imageArrayLayers = 1;
  info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
#ifdef __ANDROID__
  info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  info.preTransform = sSurfaceCaps.currentTransform;
#else
  info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
#endif
  VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
  for (VkPresentModeKHR mode : sPresentModes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
      presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
      break;
    }
  }
  info.presentMode = presentMode;

  uint32_t queueFamilyIndices[] = {sGraphicQueueFamily, sPresentQueueFamily};
  if (sGraphicQueueFamily != sPresentQueueFamily) {
    info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    info.queueFamilyIndexCount = 2;
    info.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.queueFamilyIndexCount = 1;
    info.pQueueFamilyIndices = queueFamilyIndices;
  }

  if (vkCreateSwapchainKHR(sDevice, &info, nullptr, &sSwapChain) !=
      VK_SUCCESS) {
    spdlog::error("Failed to create swapchain");
    return false;
  }
  return true;
}

static void InitSwapChainRenderTargets() {
  uint32_t imageCount = 0;
  vkGetSwapchainImagesKHR(sDevice, sSwapChain, &imageCount, nullptr);
  sSwapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(sDevice, sSwapChain, &imageCount,
                          sSwapChainImages.data());

  sSwapChainImageViews.resize(imageCount);
  for (uint32_t i = 0; i < imageCount; i++) {
    sSwapChainImageViews[i] =
        GenImageView2D(sSwapChainImages[i], sSwapChainFormat,
                       VK_IMAGE_ASPECT_COLOR_BIT);
  }

  sDepthBuffer = new Texture;
  sDepthBuffer->mFormat = VK_FORMAT_D24_UNORM_S8_UINT;
  sDepthBuffer->mImageAspectFlag =
      VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  GenImage(sDepthBuffer, static_cast<int>(sCanvasWidth),
           static_cast<int>(sCanvasHeight),
           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  sDepthBuffer->mImageView =
      GenImageView2D(sDepthBuffer->mImage, sDepthBuffer->mFormat,
                     sDepthBuffer->mImageAspectFlag);
}

static void InitSwapChainRenderPass() {
  VkAttachmentDescription attachments[2] = {};
  attachments[0].format = sSwapChainFormat;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  attachments[1].format = VK_FORMAT_D24_UNORM_S8_UINT;
  attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorRef = {0,
                                    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
  VkAttachmentReference depthRef = {
      1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;

  VkRenderPassCreateInfo rpInfo = {};
  rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  rpInfo.attachmentCount = 2;
  rpInfo.pAttachments = attachments;
  rpInfo.subpassCount = 1;
  rpInfo.pSubpasses = &subpass;

  vkCreateRenderPass(sDevice, &rpInfo, nullptr, &sSwapChainRenderPass);
}

static void InitSwapChainFBOs() {
  const auto imageCnt = sSwapChainImageViews.size();
  sSwapChainFBOs.resize(imageCnt);
  for (size_t i = 0; i < imageCnt; i++) {
    VkImageView views[] = {sSwapChainImageViews[i], sDepthBuffer->mImageView};

    VkFramebufferCreateInfo fbInfo = {};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = sSwapChainRenderPass;
    fbInfo.attachmentCount = 2;
    fbInfo.pAttachments = views;
    fbInfo.width = sCanvasWidth;
    fbInfo.height = sCanvasHeight;
    fbInfo.layers = 1;

    vkCreateFramebuffer(sDevice, &fbInfo, nullptr, &sSwapChainFBOs[i]);
  }
}

static void InitCommandPool() {
  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = sGraphicQueueFamily;

  vkCreateCommandPool(sDevice, &poolInfo, nullptr, &sCommandPool);
}

static void InitSyncResources() {
  VkSemaphoreCreateInfo semInfo = {};
  semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  vkCreateSemaphore(sDevice, &semInfo, nullptr, &sReadyToRender);
  vkCreateSemaphore(sDevice, &semInfo, nullptr, &sReadyToPresent);

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
  vkCreateFence(sDevice, &fenceInfo, nullptr, &sInFlightFence);
}

static void InitUberPassPipelineLayout() {
  VkDescriptorSetLayoutBinding bindings[4] = {};
  bindings[0].binding = 0;
  bindings[0].descriptorCount = 1;
  bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[0].stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_GEOMETRY_BIT;

  bindings[1].binding = 1;
  bindings[1].descriptorCount = 1;
  bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  bindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  bindings[2].binding = 2;
  bindings[2].descriptorCount = 1;
  bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  bindings[3].binding = 3;
  bindings[3].descriptorCount = 1;
  bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  bindings[3].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 4;
  layoutInfo.pBindings = bindings;
  vkCreateDescriptorSetLayout(sDevice, &layoutInfo, nullptr,
                              &sUberShaderParams.mDescriptorSetLayout);

  VkPushConstantRange pushRange = {};
  pushRange.offset = 0;
  pushRange.size = sizeof(float) * 16 * 2;
  pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkPipelineLayoutCreateInfo plInfo = {};
  plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  plInfo.pushConstantRangeCount = 1;
  plInfo.pPushConstantRanges = &pushRange;
  plInfo.setLayoutCount = 1;
  plInfo.pSetLayouts = &sUberShaderParams.mDescriptorSetLayout;
  vkCreatePipelineLayout(sDevice, &plInfo, nullptr,
                         &sUberShaderParams.mPipelineLayout);
}

#ifdef __ANDROID__
bool InitVulkan(ANativeWindow *window, int canvasWidth, int canvasHeight) {
#else
bool InitVulkan(GLFWwindow *window, int canvasWidth, int canvasHeight) {
#endif
  sCanvasWidth = static_cast<uint32_t>(canvasWidth);
  sCanvasHeight = static_cast<uint32_t>(canvasHeight);

  if (!InitInstance())
    return false;

  fnCreateDebugReport = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
      vkGetInstanceProcAddr(sInstance, "vkCreateDebugReportCallbackEXT"));
  fnDestroyDebugReport = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
      vkGetInstanceProcAddr(sInstance, "vkDestroyDebugReportCallbackEXT"));
  fnBeginDebugLabel = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(
      vkGetInstanceProcAddr(sInstance, "vkCmdBeginDebugUtilsLabelEXT"));
  fnEndDebugLabel = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(
      vkGetInstanceProcAddr(sInstance, "vkCmdEndDebugUtilsLabelEXT"));
  fnSetObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
      vkGetInstanceProcAddr(sInstance, "vkSetDebugUtilsObjectNameEXT"));

  if (fnCreateDebugReport) {
    InitDebugger();
  }

  if (!InitSurface(window)) {
    spdlog::error("Failed to create Vulkan surface");
    return false;
  }
  if (!InitPhysicalDevice())
    return false;
  if (!InitLogicDevice())
    return false;

  InitSurfaceProperties();
  if (!InitSwapchain()) {
    spdlog::error("Swapchain initialization failed");
    return false;
  }
  InitSwapChainRenderTargets();
  InitSwapChainRenderPass();
  InitSwapChainFBOs();
  InitCommandPool();
  InitSyncResources();
  InitUberPassPipelineLayout();

  spdlog::info("Vulkan initialized: {}x{}", canvasWidth, canvasHeight);
  return true;
}

VulkanBuffer *GenBufferObject(VkBufferUsageFlags usage,
                              VkMemoryPropertyFlagBits memProps,
                              const void *data, int size) {
  auto *buffer = new VulkanBuffer;

  VkBufferCreateInfo bufInfo = {};
  bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufInfo.size = size;
  bufInfo.usage = usage;
  if (vkCreateBuffer(sDevice, &bufInfo, nullptr, &buffer->mBuffer) !=
      VK_SUCCESS) {
    spdlog::error("Failed to create buffer");
    delete buffer;
    return nullptr;
  }

  VkMemoryRequirements memReqs;
  vkGetBufferMemoryRequirements(sDevice, buffer->mBuffer, &memReqs);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memReqs.size;
  allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, memProps);

  if (vkAllocateMemory(sDevice, &allocInfo, nullptr, &buffer->mMemory) !=
      VK_SUCCESS) {
    spdlog::error("Failed to allocate buffer memory");
    vkDestroyBuffer(sDevice, buffer->mBuffer, nullptr);
    delete buffer;
    return nullptr;
  }
  if (vkBindBufferMemory(sDevice, buffer->mBuffer, buffer->mMemory, 0) !=
      VK_SUCCESS) {
    spdlog::error("Failed to bind buffer memory");
    vkFreeMemory(sDevice, buffer->mMemory, nullptr);
    vkDestroyBuffer(sDevice, buffer->mBuffer, nullptr);
    delete buffer;
    return nullptr;
  }

  if (memProps == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && data != nullptr) {
    void *mapped = nullptr;
    vkMapMemory(sDevice, buffer->mMemory, 0, size, 0, &mapped);
    std::memcpy(mapped, data, size);
    vkUnmapMemory(sDevice, buffer->mMemory);
  }

  buffer->mSize = size;
  return buffer;
}

void BufferSubData(VulkanBuffer *buffer, const void *data, VkDeviceSize size) {
  void *dst = nullptr;
  vkMapMemory(sDevice, buffer->mMemory, 0, size, 0, &dst);
  std::memcpy(dst, data, static_cast<size_t>(size));
  vkUnmapMemory(sDevice, buffer->mMemory);
}

VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level) {
  VkCommandBuffer cb = VK_NULL_HANDLE;
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = sCommandPool;
  allocInfo.commandBufferCount = 1;
  allocInfo.level = level;
  if (vkAllocateCommandBuffers(sDevice, &allocInfo, &cb) != VK_SUCCESS) {
    spdlog::error("Failed to allocate command buffer");
    return VK_NULL_HANDLE;
  }
  return cb;
}

void BeginCommandBuffer(VkCommandBuffer cb,
                        VkCommandBufferUsageFlagBits usage) {
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = usage;
  vkBeginCommandBuffer(cb, &beginInfo);
}

bool PrepareSwapChainFrame(uint32_t *outImageIndex) {
  vkWaitForFences(sDevice, 1, &sInFlightFence, VK_TRUE, UINT64_MAX);

  VkResult acquire =
      vkAcquireNextImageKHR(sDevice, sSwapChain, UINT64_MAX, sReadyToRender,
                            VK_NULL_HANDLE, outImageIndex);
  if (acquire != VK_SUCCESS && acquire != VK_SUBOPTIMAL_KHR) {
    spdlog::warn("vkAcquireNextImageKHR failed: {}", static_cast<int>(acquire));
    return false;
  }
  return true;
}

void CmdBeginSwapChainRenderPass(VkCommandBuffer cb, uint32_t imageIndex) {
  if (imageIndex >= sSwapChainFBOs.size()) {
    spdlog::error("Swapchain image index out of range");
    return;
  }

  VkClearValue clearValues[2];
  clearValues[0].color = {{0.1f, 0.4f, 0.6f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo rpBegin = {};
  rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  rpBegin.renderPass = sSwapChainRenderPass;
  rpBegin.framebuffer = sSwapChainFBOs[imageIndex];
  rpBegin.clearValueCount = 2;
  rpBegin.pClearValues = clearValues;
  rpBegin.renderArea.offset = {0, 0};
  rpBegin.renderArea.extent = {sCanvasWidth, sCanvasHeight};

  vkCmdBeginRenderPass(cb, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);
}

void SubmitAndPresentSwapChainFrame(VkCommandBuffer cb, uint32_t imageIndex) {
  vkResetFences(sDevice, 1, &sInFlightFence);

  VkPipelineStageFlags waitStage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cb;
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = &sReadyToRender;
  submitInfo.pWaitDstStageMask = &waitStage;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &sReadyToPresent;
  vkQueueSubmit(sGraphicQueue, 1, &submitInfo, sInFlightFence);

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &sReadyToPresent;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &sSwapChain;
  presentInfo.pImageIndices = &imageIndex;
  vkQueuePresentKHR(sPresentQueue, &presentInfo);

  vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cb);
}

void CmdBarrierAfterComputeWrites(VkCommandBuffer cb) {
  VkMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
                          VK_ACCESS_SHADER_WRITE_BIT |
                          VK_ACCESS_UNIFORM_READ_BIT;
  vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0,
                       nullptr, 0, nullptr);
}

void CmdBarrierComputeToIndirectAndDraw(VkCommandBuffer cb) {
  VkMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barrier.dstAccessMask =
      VK_ACCESS_INDIRECT_COMMAND_READ_BIT | VK_ACCESS_SHADER_READ_BIT |
      VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_UNIFORM_READ_BIT;
  vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                           VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0, 1, &barrier, 0, nullptr, 0, nullptr);
}

void CmdBarrierFragmentStorageToCompute(VkCommandBuffer cb) {
  VkMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
  barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
  barrier.dstAccessMask =
      VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
  vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &barrier, 0,
                       nullptr, 0, nullptr);
}

void CmdBarrierComputeToFragmentSample(VkCommandBuffer cb) {
  vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 0, nullptr);
}

VkQueue GetGraphicQueue() { return sGraphicQueue; }
VkDevice GetVulkanDevice() { return sDevice; }
VkCommandPool GetVulkanCommandPool() { return sCommandPool; }
VkPhysicalDevice GetPhysicalDevice() { return sGPU; }
VkRenderPass GetSwapChainRenderPass() { return sSwapChainRenderPass; }
ShaderParameterDescription *GetUberPassShaderParameterDescription() {
  return &sUberShaderParams;
}
VkFramebuffer *GetSwapChainFrameBuffers() {
  return sSwapChainFBOs.empty() ? nullptr : sSwapChainFBOs.data();
}

void GenImage(Texture *outTex, int width, int height, VkImageUsageFlags usage,
              VkMemoryPropertyFlagBits memProps) {
  VkImageCreateInfo imgInfo = {};
  imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgInfo.imageType = VK_IMAGE_TYPE_2D;
  imgInfo.format = outTex->mFormat;
  imgInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                    1};
  imgInfo.mipLevels = 1;
  imgInfo.arrayLayers = 1;
  imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgInfo.usage = usage;
  imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (vkCreateImage(sDevice, &imgInfo, nullptr, &outTex->mImage) !=
      VK_SUCCESS) {
    spdlog::error("Failed to create image");
    return;
  }

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(sDevice, outTex->mImage, &memReqs);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memReqs.size;
  allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, memProps);

  if (vkAllocateMemory(sDevice, &allocInfo, nullptr, &outTex->mMemory) !=
      VK_SUCCESS) {
    spdlog::error("Failed to allocate image memory");
    vkDestroyImage(sDevice, outTex->mImage, nullptr);
    outTex->mImage = VK_NULL_HANDLE;
    return;
  }
  if (vkBindImageMemory(sDevice, outTex->mImage, outTex->mMemory, 0) !=
      VK_SUCCESS) {
    spdlog::error("Failed to bind image memory");
    vkFreeMemory(sDevice, outTex->mMemory, nullptr);
    vkDestroyImage(sDevice, outTex->mImage, nullptr);
    outTex->mImage = VK_NULL_HANDLE;
    outTex->mMemory = VK_NULL_HANDLE;
  }
}

void GenImageWithMipLevels(Texture2D *outTex, int width, int height,
                           uint32_t mipLevels, VkImageUsageFlags usage,
                           VkMemoryPropertyFlagBits memProps) {
  VkImageCreateInfo imgInfo = {};
  imgInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imgInfo.imageType = VK_IMAGE_TYPE_2D;
  imgInfo.format = outTex->mFormat;
  imgInfo.extent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height),
                    1};
  imgInfo.mipLevels = mipLevels;
  imgInfo.arrayLayers = 1;
  imgInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imgInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imgInfo.usage = usage;
  imgInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imgInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  if (vkCreateImage(sDevice, &imgInfo, nullptr, &outTex->mImage) !=
      VK_SUCCESS) {
    spdlog::error("Failed to create mipmapped image");
    return;
  }

  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(sDevice, outTex->mImage, &memReqs);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memReqs.size;
  allocInfo.memoryTypeIndex = FindMemoryType(memReqs.memoryTypeBits, memProps);

  if (vkAllocateMemory(sDevice, &allocInfo, nullptr, &outTex->mMemory) !=
      VK_SUCCESS) {
    spdlog::error("Failed to allocate mipmapped image memory");
    vkDestroyImage(sDevice, outTex->mImage, nullptr);
    outTex->mImage = VK_NULL_HANDLE;
    return;
  }
  if (vkBindImageMemory(sDevice, outTex->mImage, outTex->mMemory, 0) !=
      VK_SUCCESS) {
    spdlog::error("Failed to bind mipmapped image memory");
    vkFreeMemory(sDevice, outTex->mMemory, nullptr);
    vkDestroyImage(sDevice, outTex->mImage, nullptr);
    outTex->mImage = VK_NULL_HANDLE;
    outTex->mMemory = VK_NULL_HANDLE;
  }
}

VkImageView GenImageView2DMipRange(VkImage image, VkFormat format,
                                   VkImageAspectFlags aspect,
                                   uint32_t baseMipLevel,
                                   uint32_t levelCount) {
  VkImageViewCreateInfo viewInfo = {};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  viewInfo.subresourceRange.aspectMask = aspect;
  viewInfo.subresourceRange.baseMipLevel = baseMipLevel;
  viewInfo.subresourceRange.levelCount = levelCount;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView view = VK_NULL_HANDLE;
  if (vkCreateImageView(sDevice, &viewInfo, nullptr, &view) != VK_SUCCESS) {
    spdlog::error("Failed to create image view");
  }
  return view;
}

VkImageView GenImageView2D(VkImage image, VkFormat format,
                           VkImageAspectFlags aspect) {
  return GenImageView2DMipRange(image, format, aspect, 0, 1);
}

void SubmitOneTimeCommandBuffer(VkCommandBuffer cb) {
  VkFence fence = VK_NULL_HANDLE;
  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  if (vkCreateFence(sDevice, &fenceInfo, nullptr, &fence) != VK_SUCCESS) {
    spdlog::error("Failed to create fence for one-time submit");
    vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cb);
    return;
  }

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cb;
  vkQueueSubmit(sGraphicQueue, 1, &submitInfo, fence);
  vkWaitForFences(sDevice, 1, &fence, VK_TRUE, UINT64_MAX);
  vkDestroyFence(sDevice, fence, nullptr);
  vkFreeCommandBuffers(sDevice, sCommandPool, 1, &cb);
}

VkSampler GenSampler(VkFilter minFilter, VkFilter magFilter,
                     VkSamplerAddressMode wrapU, VkSamplerAddressMode wrapV,
                     VkSamplerAddressMode wrapW) {
  VkSamplerCreateInfo samplerInfo = {};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.minFilter = minFilter;
  samplerInfo.magFilter = magFilter;
  samplerInfo.addressModeU = wrapU;
  samplerInfo.addressModeV = wrapV;
  samplerInfo.addressModeW = wrapW;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

  VkSampler sampler = VK_NULL_HANDLE;
  vkCreateSampler(sDevice, &samplerInfo, nullptr, &sampler);
  return sampler;
}

VkPipeline
CreatePSO(VkRenderPass renderPass, VkPrimitiveTopology topology,
          const std::vector<VkVertexInputBindingDescription> &bindings,
          const std::vector<VkVertexInputAttributeDescription> &attributes,
          VkShaderModule vs, VkShaderModule fs) {
  VkPipelineVertexInputStateCreateInfo vertexInput = {};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount =
      static_cast<uint32_t>(bindings.size());
  vertexInput.pVertexBindingDescriptions = bindings.data();
  vertexInput.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributes.size());
  vertexInput.pVertexAttributeDescriptions = attributes.data();

  VkPipelineDynamicStateCreateInfo dynamicState = {};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;

  // Note: flip viewport Y for Vulkan coordinate
  VkViewport viewport = {};
  viewport.x = 0.0f;
  viewport.y = static_cast<float>(sCanvasHeight);
  viewport.width = static_cast<float>(sCanvasWidth);
  viewport.height = -static_cast<float>(sCanvasHeight);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.extent = {sCanvasWidth, sCanvasHeight};

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = topology;

  VkPipelineRasterizationStateCreateInfo raster = {};
  raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  raster.polygonMode = VK_POLYGON_MODE_FILL;
  raster.lineWidth = 1.0f;
  raster.cullMode = VK_CULL_MODE_BACK_BIT;
  raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

  VkPipelineMultisampleStateCreateInfo multisample = {};
  multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisample.minSampleShading = 1.0f;

  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_TRUE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

  VkPipelineColorBlendAttachmentState blendAttachment = {};
  blendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlend = {};
  colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlend.attachmentCount = 1;
  colorBlend.pAttachments = &blendAttachment;

  VkPipelineShaderStageCreateInfo stages[2] = {};
  stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  stages[0].module = vs;
  stages[0].pName = "main";
  stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  stages[1].module = fs;
  stages[1].pName = "main";

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = stages;
  pipelineInfo.basePipelineIndex = -1;
  pipelineInfo.pVertexInputState = &vertexInput;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pRasterizationState = &raster;
  pipelineInfo.pMultisampleState = &multisample;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlend;
  pipelineInfo.layout = sUberShaderParams.mPipelineLayout;

  VkPipeline pipeline = VK_NULL_HANDLE;
  vkCreateGraphicsPipelines(sDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                            &pipeline);
  return pipeline;
}

VkShaderModule LoadShaderModule(const char *filePath) {
  std::vector<unsigned char> content;
  size_t fileSize = 0;

#ifdef __ANDROID__
  AAsset *asset =
      AAssetManager_open(sAssetManager, filePath, AASSET_MODE_BUFFER);
  if (!asset) {
    spdlog::error("Failed to open shader asset: {}", filePath);
    return VK_NULL_HANDLE;
  }
  fileSize = static_cast<size_t>(AAsset_getLength(asset));
  content.resize(fileSize);
  if (fileSize > 0) {
    int read = AAsset_read(asset, content.data(), fileSize);
    if (read < 0 || static_cast<size_t>(read) != fileSize) {
      spdlog::error("Incomplete shader asset read: {}", filePath);
      AAsset_close(asset);
      return VK_NULL_HANDLE;
    }
  }
  AAsset_close(asset);
#else
  FILE *file = std::fopen(filePath, "rb");
  if (!file) {
    spdlog::error("Failed to open shader: {}", filePath);
    return VK_NULL_HANDLE;
  }

  if (std::fseek(file, 0, SEEK_END) != 0) {
    spdlog::error("Failed to seek shader: {}", filePath);
    std::fclose(file);
    return VK_NULL_HANDLE;
  }

  const long fileSizeLong = std::ftell(file);
  if (fileSizeLong < 0) {
    spdlog::error("Failed to get shader size: {}", filePath);
    std::fclose(file);
    return VK_NULL_HANDLE;
  }

  fileSize = static_cast<size_t>(fileSizeLong);
  if (std::fseek(file, 0, SEEK_SET) != 0) {
    spdlog::error("Failed to rewind shader: {}", filePath);
    std::fclose(file);
    return VK_NULL_HANDLE;
  }

  content.resize(fileSize);
  if (fileSize > 0) {
    const size_t read = std::fread(content.data(), 1, fileSize, file);
    if (read != fileSize) {
      spdlog::error("Incomplete shader read: {} ({}/{})", filePath, read,
                    fileSize);
      std::fclose(file);
      return VK_NULL_HANDLE;
    }
  }
  std::fclose(file);
#endif

  VkShaderModuleCreateInfo moduleInfo = {};
  moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  moduleInfo.pCode = reinterpret_cast<const uint32_t *>(content.data());
  moduleInfo.codeSize = fileSize;

  VkShaderModule shader = VK_NULL_HANDLE;
  if (vkCreateShaderModule(sDevice, &moduleInfo, nullptr, &shader) !=
      VK_SUCCESS) {
    spdlog::error("Failed to create shader module: {}", filePath);
  }
  return shader;
}

void TransferImageLayout(VkCommandBuffer cb, VkImage image,
                         VkImageSubresourceRange subresourceRange,
                         VkImageLayout oldLayout, VkAccessFlags oldAccess,
                         VkPipelineStageFlags oldStage, VkImageLayout newLayout,
                         VkAccessFlags newAccess,
                         VkPipelineStageFlags newStage) {
  VkImageMemoryBarrier barrier = {};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcAccessMask = oldAccess;
  barrier.dstAccessMask = newAccess;
  barrier.image = image;
  barrier.subresourceRange = subresourceRange;

  vkCmdPipelineBarrier(cb, oldStage, newStage, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);
}

void BeginEvent(VkCommandBuffer cb, const char *name) {
  if (!fnBeginDebugLabel)
    return;
  VkDebugUtilsLabelEXT label = {};
  label.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
  label.pLabelName = name;
  label.color[0] = label.color[1] = label.color[2] = label.color[3] = 1.0f;
  fnBeginDebugLabel(cb, &label);
}

void EndEvent(VkCommandBuffer cb) {
  if (!fnEndDebugLabel)
    return;
  fnEndDebugLabel(cb);
}

void SetObjectName(VkObjectType type, void *object, const char *name) {
  if (!fnSetObjectName)
    return;
  VkDebugUtilsObjectNameInfoEXT nameInfo = {};
  nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
  nameInfo.objectType = type;
  nameInfo.objectHandle = reinterpret_cast<uint64_t>(object);
  nameInfo.pObjectName = name;
  fnSetObjectName(sDevice, &nameInfo);
}

void GlobalConstants::SetProjectionMatrix(const float *matrix) {
  std::memcpy(mProjectionMatrix, matrix, sizeof(mProjectionMatrix));
}

void GlobalConstants::SetViewMatrix(float *matrix) {
  // Zero out translation for translated world space
  matrix[12] = 0.0f;
  matrix[13] = 0.0f;
  matrix[14] = 0.0f;
  std::memcpy(mViewMatrix, matrix, sizeof(mViewMatrix));
}

void GlobalConstants::SetModelMatrix(const float *matrix) {
  std::memcpy(mModelMatrix, matrix, sizeof(mModelMatrix));
}

void GlobalConstants::SetMisc0(unsigned int x, unsigned int y, unsigned int z,
                               unsigned int w) {
  mMisc0[0] = x;
  mMisc0[1] = y;
  mMisc0[2] = z;
  mMisc0[3] = w;
}

void GlobalConstants::SetCameraPositionWS(float x, float y, float z, float w) {
  mCameraPositionWS[0] = x;
  mCameraPositionWS[1] = y;
  mCameraPositionWS[2] = z;
  mCameraPositionWS[3] = w;
}

void GlobalConstants::SetCameraViewDirectionWS(float x, float y, float z,
                                               float w) {
  mViewDirectionWS[0] = x;
  mViewDirectionWS[1] = y;
  mViewDirectionWS[2] = z;
  mViewDirectionWS[3] = w;
}
