#include "rhi.h"
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "misc/logger.h"
#include "render/window.h"

namespace Nano
{

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT      flags,
                                                        VkDebugReportObjectTypeEXT objectType,
                                                        uint64_t                   object,
                                                        size_t /* location */,
                                                        int32_t     messageCode,
                                                        const char* pLayerPrefix,
                                                        const char* pMessage,
                                                        void* /* pUserData */)
    {
        const char* ignored_messages[] = {
            "Loader Message",
            "Device Extension:",
        };

        for (const char* ignored : ignored_messages)
        {
            if (std::strstr(pMessage, ignored) != nullptr)
            {
                return VK_FALSE;
            }
        }

        const char* object_type_name = "Unknown";
        switch (objectType)
        {
            case VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT:
                object_type_name = "Instance";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT:
                object_type_name = "PhysicalDevice";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT:
                object_type_name = "Device";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT:
                object_type_name = "Queue";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
                object_type_name = "Semaphore";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT:
                object_type_name = "CommandBuffer";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT:
                object_type_name = "Fence";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
                object_type_name = "DeviceMemory";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
                object_type_name = "Buffer";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
                object_type_name = "Image";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
                object_type_name = "Event";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
                object_type_name = "QueryPool";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
                object_type_name = "BufferView";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
                object_type_name = "ImageView";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT:
                object_type_name = "ShaderModule";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT:
                object_type_name = "PipelineCache";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT:
                object_type_name = "PipelineLayout";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
                object_type_name = "RenderPass";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
                object_type_name = "Pipeline";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT:
                object_type_name = "DescriptorSetLayout";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
                object_type_name = "Sampler";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
                object_type_name = "DescriptorPool";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
                object_type_name = "DescriptorSet";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
                object_type_name = "Framebuffer";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
                object_type_name = "CommandPool";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_SURFACE_KHR_EXT:
                object_type_name = "SurfaceKHR";
                break;
            case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
                object_type_name = "SwapchainKHR";
                break;
            default:
                break;
        }

        if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        {
            ERROR("Vulkan Error [%s] (%s 0x%llx, Code %d): %s",
                  pLayerPrefix,
                  object_type_name,
                  static_cast<unsigned long long>(object),
                  messageCode,
                  pMessage);
        }
        else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
        {
            WARN("Vulkan Warning [%s] (%s 0x%llx, Code %d): %s",
                 pLayerPrefix,
                 object_type_name,
                 static_cast<unsigned long long>(object),
                 messageCode,
                 pMessage);
        }
        else if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
        {
            WARN("Vulkan Performance [%s] (%s 0x%llx): %s",
                 pLayerPrefix,
                 object_type_name,
                 static_cast<unsigned long long>(object),
                 pMessage);
        }
        else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        {
            DEBUG("Vulkan Info [%s] (%s): %s", pLayerPrefix, object_type_name, pMessage);
        }
        else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        {
            DEBUG("Vulkan Debug [%s]: %s", pLayerPrefix, pMessage);
        }

        return VK_FALSE;
    }

    RHI::RHI()
    {
        if (initInstance() == false)
            FATAL("Failed when init vulkan instance.");
        DEBUG("Successfully initialize vulkan instance.");

        if (initDebugger() == false)
            WARN("Failed when init vulkan debugger.");
        DEBUG("Successfully initialize or ignore vulkan debugger.");

        if (initSurface() == false)
            FATAL("Failed when init vulkan surface");
        DEBUG("Successfully initialize vulkan surface.");

        if (initPhysicalDevice() == false)
            FATAL("Failed when init vulkan physical device");
        DEBUG("Successfully initialize vulkan physical device.");

        if (initLogicalDevice() == false)
            FATAL("Failed when init vulkan logical device");
        DEBUG("Successfully initialize vulkan logical device.");

        if (initSurfaceProperties() == false)
            FATAL("Failed when init vulkan surface properties");
        DEBUG("Successfully initialize vulkan surface properties.");
    }

    RHI::~RHI() noexcept
    {
        if (m_device != VK_NULL_HANDLE)
        {
            vkDeviceWaitIdle(m_device);
            vkDestroyDevice(m_device, nullptr);
            m_device = VK_NULL_HANDLE;

            DEBUG("  Destroyed logical device");
        }

        if (m_surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
            m_surface = VK_NULL_HANDLE;

            DEBUG("  Destroyed surface");
        }

        if (m_debug_report_callback != VK_NULL_HANDLE && m_vkDestroyDebugReportCallbackEXT)
        {
            m_vkDestroyDebugReportCallbackEXT(m_instance, m_debug_report_callback, nullptr);
            m_debug_report_callback = VK_NULL_HANDLE;

            DEBUG("  Destroyed debug report callback");
        }

        if (m_surface_formats != nullptr)
        {
            delete[] m_surface_formats;
            m_surface_formats = nullptr;

            DEBUG("  Released surface formats array");
        }

        if (m_surface_present_modes != nullptr)
        {
            delete[] m_surface_present_modes;
            m_surface_present_modes = nullptr;

            DEBUG("  Released surface present modes array");
        }

        if (m_prefered_layers != nullptr)
        {
            for (uint32_t i = 0; i < m_prefered_layer_cnt; ++i)
            {
                if (m_prefered_layers[i] != nullptr)
                {
                    delete[] m_prefered_layers[i];
                    m_prefered_layers[i] = nullptr;
                }
            }
            delete[] m_prefered_layers;
            m_prefered_layers = nullptr;

            DEBUG("  Released %u preferred layers", m_prefered_layer_cnt);
            m_prefered_layer_cnt = 0;
        }

        if (m_instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;

            DEBUG("  Destroyed Vulkan instance");
        }

        // Note: m_enabled_instance_exts is owned by GLFW, don't free it
        // Note: m_enabled_device_exts points to a local array, don't free it
    }

    bool RHI::initInstance()
    {
        m_enabled_instance_exts = glfwGetRequiredInstanceExtensions(&m_enabled_instance_ext_cnt);
        if (!m_enabled_instance_exts)
        {
            ERROR("Failed to get GLFW required instance extensions");
            return false;
        }

        VkApplicationInfo vkApplicationInfo  = {};
        vkApplicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        vkApplicationInfo.pApplicationName   = "Nano Virtualized Geomery";
        vkApplicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
        vkApplicationInfo.pEngineName        = "Nano";
        vkApplicationInfo.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
        vkApplicationInfo.apiVersion         = VK_API_VERSION_1_2;

        VkInstanceCreateInfo vkInstanceCreateInfo    = {};
        vkInstanceCreateInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        vkInstanceCreateInfo.pApplicationInfo        = &vkApplicationInfo;
        vkInstanceCreateInfo.enabledExtensionCount   = m_enabled_instance_ext_cnt;
        vkInstanceCreateInfo.ppEnabledExtensionNames = m_enabled_instance_exts;

        uint32_t layer_cnt;
        vkEnumerateInstanceLayerProperties(&layer_cnt, nullptr);
        VkLayerProperties* layer_properties = new VkLayerProperties[layer_cnt];
        m_prefered_layers                   = new char*[layer_cnt];
        vkEnumerateInstanceLayerProperties(&layer_cnt, layer_properties);

        uint32_t prefered_layer_index = 0;
        for (uint32_t i = 0; i < layer_cnt; ++i)
        {
            DEBUG("detect layer : %s", layer_properties[i].layerName);
            if (std::strstr(layer_properties[i].layerName, "validation") != nullptr)
            {
                m_prefered_layers[prefered_layer_index] = new char[64];
                std::memset(m_prefered_layers[prefered_layer_index], 0, 64);
                std::strncpy(m_prefered_layers[prefered_layer_index++], layer_properties[i].layerName, 63);
            }
        }
        delete[] layer_properties;

        m_prefered_layer_cnt = prefered_layer_index;
#ifdef DEBUG
        vkInstanceCreateInfo.enabledLayerCount   = m_prefered_layer_cnt;
        vkInstanceCreateInfo.ppEnabledLayerNames = m_prefered_layers;
#endif // DEBUG

        if (vkCreateInstance(&vkInstanceCreateInfo, nullptr, &m_instance) != VK_SUCCESS)
        {
            ERROR("Failed to create Vulkan Instance.");
            return false;
        }

        return true;
    }

    bool RHI::initDebugger()
    {
        m_vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(m_instance, "vkCreateDebugReportCallbackEXT"));
        m_vkDestroyDebugReportCallbackEXT = reinterpret_cast<PFN_vkDestroyDebugReportCallbackEXT>(
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugReportCallbackEXT"));
        if (!m_vkCreateDebugReportCallbackEXT || !m_vkDestroyDebugReportCallbackEXT)
        {
            WARN("vkCreate(Destroy)DebugReportCallbackEXT is not available.");
            return false;
        }

        VkDebugReportCallbackCreateInfoEXT vkDebugReportCallbackCreateInfo = {};
        vkDebugReportCallbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        vkDebugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                                                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
        vkDebugReportCallbackCreateInfo.pfnCallback = debugCallback;

        if (m_vkCreateDebugReportCallbackEXT(
                m_instance, &vkDebugReportCallbackCreateInfo, nullptr, &m_debug_report_callback) != VK_SUCCESS)
        {
            WARN("Failed to create debug report callback.");
            return false;
        }

        return true;
    }

    bool RHI::initSurface()
    {
        if (glfwCreateWindowSurface(m_instance, Window::instance().getGLFWWindow(), nullptr, &m_surface) != VK_SUCCESS)
        {
            ERROR("Failed to create window surface.");
            return false;
        }
        return true;
    }

    bool RHI::initPhysicalDevice()
    {
        uint32_t device_cnt = 0;
        vkEnumeratePhysicalDevices(m_instance, &device_cnt, nullptr);
        VkPhysicalDevice* devices = new VkPhysicalDevice[device_cnt];
        vkEnumeratePhysicalDevices(m_instance, &device_cnt, devices);

        for (uint32_t i = 0; i < device_cnt; ++i)
        {
            int graphic_queue_family_index = -1;
            int present_queue_family_index = -1;

            // log each GPU info
            VkPhysicalDevice           curr_device = devices[i];
            VkPhysicalDeviceProperties curr_device_props {};
            vkGetPhysicalDeviceProperties(curr_device, &curr_device_props);

            DEBUG("GPU %u:", i);
            DEBUG("  Name : %s", curr_device_props.deviceName);

            const char* device_type_str = "Unknown";
            switch (curr_device_props.deviceType)
            {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    device_type_str = "Other";
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    device_type_str = "Integrated GPU";
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    device_type_str = "Discrete GPU (Recommended)";
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    device_type_str = "Virtual GPU";
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    device_type_str = "CPU";
                    break;
                default:
                    break;
            }
            DEBUG("  Type : %s", device_type_str);
            DEBUG("  API Version : %u.%u.%u",
                  VK_VERSION_MAJOR(curr_device_props.apiVersion),
                  VK_VERSION_MINOR(curr_device_props.apiVersion),
                  VK_VERSION_PATCH(curr_device_props.apiVersion));
            DEBUG("  Driver Version : %u.%u.%u",
                  VK_VERSION_MAJOR(curr_device_props.driverVersion),
                  VK_VERSION_MINOR(curr_device_props.driverVersion),
                  VK_VERSION_PATCH(curr_device_props.driverVersion));
            DEBUG("  Vendor ID : 0x%04X", curr_device_props.vendorID);
            DEBUG("  Device ID : 0x%04X", curr_device_props.deviceID);

            // find queues we need
            uint32_t device_queue_family_prop_cnt = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(curr_device, &device_queue_family_prop_cnt, nullptr);
            VkQueueFamilyProperties* device_queue_family_props =
                new VkQueueFamilyProperties[device_queue_family_prop_cnt];
            vkGetPhysicalDeviceQueueFamilyProperties(
                curr_device, &device_queue_family_prop_cnt, device_queue_family_props);

            for (uint32_t j = 0; j < device_queue_family_prop_cnt; ++j)
            {
                if (device_queue_family_props[j].queueCount > 0 &&
                    device_queue_family_props[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    graphic_queue_family_index = j;

                VkBool32 support_present = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(curr_device, j, m_surface, &support_present);
                if (support_present && device_queue_family_props[j].queueCount > 0)
                    present_queue_family_index = j;

                if (graphic_queue_family_index != -1 && present_queue_family_index != -1)
                {
                    m_physical_device            = curr_device;
                    m_graphic_queue_family_index = static_cast<uint32_t>(graphic_queue_family_index);
                    m_present_queue_family_index = static_cast<uint32_t>(present_queue_family_index);
                    delete[] device_queue_family_props;
                    delete[] devices;
                    return true;
                }
            }

            delete[] device_queue_family_props;
        }

        delete[] devices;
        ERROR("No available device detected.");
        return false;
    }

    bool RHI::initLogicalDevice()
    {
        VkDeviceQueueCreateInfo vkDeviceQueueCreateInfos[2];
        float                   p                     = 1.0f;
        int                     queue_create_info_cnt = 2;
        if (m_graphic_queue_family_index == m_present_queue_family_index)
        {
            vkDeviceQueueCreateInfos[0]                  = {};
            vkDeviceQueueCreateInfos[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            vkDeviceQueueCreateInfos[0].queueFamilyIndex = m_graphic_queue_family_index;
            vkDeviceQueueCreateInfos[0].queueCount       = 1;
            vkDeviceQueueCreateInfos[0].pQueuePriorities = &p;
            queue_create_info_cnt                        = 1;
        }
        else
        {
            vkDeviceQueueCreateInfos[0]                  = {};
            vkDeviceQueueCreateInfos[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            vkDeviceQueueCreateInfos[0].queueFamilyIndex = m_graphic_queue_family_index;
            vkDeviceQueueCreateInfos[0].queueCount       = 1;
            vkDeviceQueueCreateInfos[0].pQueuePriorities = &p;
            vkDeviceQueueCreateInfos[1]                  = {};
            vkDeviceQueueCreateInfos[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            vkDeviceQueueCreateInfos[1].queueFamilyIndex = m_present_queue_family_index;
            vkDeviceQueueCreateInfos[1].queueCount       = 1;
            vkDeviceQueueCreateInfos[1].pQueuePriorities = &p;
            queue_create_info_cnt                        = 2;
        }

        VkDeviceCreateInfo vkDeviceCreateInfo   = {};
        vkDeviceCreateInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        vkDeviceCreateInfo.queueCreateInfoCount = queue_create_info_cnt;
        vkDeviceCreateInfo.pQueueCreateInfos    = vkDeviceQueueCreateInfos;

#ifdef DEBUG
        vkDeviceCreateInfo.enabledLayerCount   = m_prefered_layer_cnt;
        vkDeviceCreateInfo.ppEnabledLayerNames = m_prefered_layers;
#endif // DEBUG

        m_enabled_device_ext_cnt                   = 1;
        const char* device_exts[]                  = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
        m_enabled_device_exts                      = device_exts;
        vkDeviceCreateInfo.enabledExtensionCount   = m_enabled_device_ext_cnt;
        vkDeviceCreateInfo.ppEnabledExtensionNames = m_enabled_device_exts;

        VkPhysicalDeviceFeatures2 features2 = {};
        features2.sType                     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        VkPhysicalDeviceShaderAtomicInt64Features shader_atomic_i64_features = {};
        shader_atomic_i64_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES;
        features2.pNext                  = &shader_atomic_i64_features;
        vkGetPhysicalDeviceFeatures2(m_physical_device, &features2);

        bool support_shader_i64 = features2.features.shaderInt64;
        if (!support_shader_i64)
        {
            ERROR("Device not support int64 type in shader.");
            return false;
        }
        bool support_buffer_i64_atomics = shader_atomic_i64_features.shaderBufferInt64Atomics;
        if (!support_buffer_i64_atomics)
        {
            ERROR("Device not support int64 atomic type in buffer.");
            return false;
        }

        if (vkCreateDevice(m_physical_device, &vkDeviceCreateInfo, nullptr, &m_device) != VK_SUCCESS)
        {
            return false;
        }

        vkGetDeviceQueue(m_device, m_graphic_queue_family_index, 0, &m_graphic_queue);
        vkGetDeviceQueue(m_device, m_present_queue_family_index, 0, &m_present_queue);

        return true;
    }

    bool RHI::initSurfaceProperties()
    {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physical_device, m_surface, &m_surface_capabilities);

        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &m_surface_format_cnt, nullptr);
        m_surface_formats = new VkSurfaceFormatKHR[m_surface_format_cnt];
        vkGetPhysicalDeviceSurfaceFormatsKHR(m_physical_device, m_surface, &m_surface_format_cnt, m_surface_formats);

        vkGetPhysicalDeviceSurfacePresentModesKHR(m_physical_device, m_surface, &m_surface_present_mode_cnt, nullptr);
        m_surface_present_modes = new VkPresentModeKHR[m_surface_present_mode_cnt];
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            m_physical_device, m_surface, &m_surface_present_mode_cnt, m_surface_present_modes);

        return true;
    }

} // namespace Nano