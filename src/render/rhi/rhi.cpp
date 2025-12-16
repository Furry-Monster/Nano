#include "rhi.h"
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "GLFW/glfw3.h"
#include "misc/logger.h"
#include "render/window.h"

namespace Nano
{

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugReportFlagsEXT      flags,
                                                        VkDebugReportObjectTypeEXT objectType,
                                                        uint64_t                   object,
                                                        size_t                     location,
                                                        int32_t                    messageCode,
                                                        const char*                pLayerPrefix,
                                                        const char*                pMessage,
                                                        void*                      pUserData)
    {
        DEBUG("vulkan debug report : %s", pMessage);
        return VK_FALSE;
    }

    RHI::RHI()
    {
        if (initInstance() == false)
            FATAL("Failed when init vulkan instance.");

        if (initDebugger() == false)
            WARN("Failed when init vulkan debugger.");

        if (initSurface() == false)
            FATAL("Failed when init vulkan surface");

        if (initPhysicalDevice() == false)
            FATAL("Failed when init vulkan physical device");

        if (initDevice() == false)
            FATAL("Failed when init vulkan logical device");
    }

    RHI::~RHI() noexcept
    {
        if (m_debug_report_callback != VK_NULL_HANDLE && m_vkDestroyDebugReportCallbackEXT)
            m_vkDestroyDebugReportCallbackEXT(m_instance, m_debug_report_callback, nullptr);
    }

    bool RHI::initInstance()
    {
        m_enabled_exts = glfwGetRequiredInstanceExtensions(&m_enabled_ext_cnt);
        if (!m_enabled_exts)
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
        vkInstanceCreateInfo.enabledExtensionCount   = m_enabled_ext_cnt;
        vkInstanceCreateInfo.ppEnabledExtensionNames = m_enabled_exts;

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
        if (!m_vkCreateDebugReportCallbackEXT)
        {
            WARN("vkCreateDebugReportCallbackEXT is not available.");
            return false;
        }

        VkDebugReportCallbackCreateInfoEXT vkDebugReportCallbackCreateInfo = {};
        vkDebugReportCallbackCreateInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        vkDebugReportCallbackCreateInfo.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
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

        int graphic_queue_family_index = -1;
        int present_queue_family_index = -1;
        for (uint32_t i = 0; i < device_cnt; ++i)
        {
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

                if (graphic_queue_family_index == -1 && present_queue_family_index != -1)
                {
                    m_physical_device            = curr_device;
                    m_graphic_queue_family_index = static_cast<uint32_t>(graphic_queue_family_index);
                    m_present_queue_family_index = static_cast<uint32_t>(present_queue_family_index);
                    return true;
                }
            }
        }

        ERROR("No avaliable device detected.");
        return false;
    }

    bool RHI::initDevice() { return true; }
} // namespace Nano