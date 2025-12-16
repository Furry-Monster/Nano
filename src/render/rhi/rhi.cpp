#include "rhi.h"
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan_core.h>
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

    RHI::~RHI() noexcept {}

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
            DEBUG("detect layer : " + layer_properties[i].layerName);
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
        VkDebugReportCallbackCreateInfoEXT vkDebugReportCallbackCreateInfo = {};
        vkDebugReportCallbackCreateInfo.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
        vkDebugReportCallbackCreateInfo.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
        vkDebugReportCallbackCreateInfo.pfnCallback = debugCallback;

        if (vkCreateDebugReportCallbackEXT(
                m_instance, &vkDebugReportCallbackCreateInfo, nullptr, &m_debug_report_callback) != VK_SUCCESS)
        {
            WARN("Failed to create debug report callback.");
            return false;
        }

        return true;
    }

    bool RHI::initSurface()
    {
        if (glfwCreateWindowSurface(m_instance, g_window.getGLFWWindow(), nullptr, &m_surface) != VK_SUCCESS)
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
        VkPhysicalDevice* all_vk_gpus = new VkPhysicalDevice[device_cnt];
        vkEnumeratePhysicalDevices(m_instance, &device_cnt, all_vk_gpus);

        int graphic_queue_family_index = -1;
        int present_queue_family_index = -1;
        for (uint32_t i = 0; i < device_cnt; ++i)
        {
            VkPhysicalDevice           device = all_vk_gpus[i];
            VkPhysicalDeviceProperties device_props {};
            vkGetPhysicalDeviceProperties(device, &device_props);
            DEBUG("Detected GPU :\n");
            DEBUG("\tName: %s\n", device_props.deviceName);
            switch (device_props.deviceType)
            {
                case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                    DEBUG("\tType: Unknown type\n");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    DEBUG("\tType: Integrated GPU\n");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    DEBUG("\tType: Discrete GPU (Recommanded)\n");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    DEBUG("\tType: Virtual GPU\n");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    DEBUG("\tType: CPU\n");
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_MAX_ENUM:
                    break;
            }

            uint32_t queue_family_prop_cnt = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_prop_cnt, nullptr);
            VkQueueFamilyProperties* queue_family_props = new VkQueueFamilyProperties[queue_family_prop_cnt];
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_prop_cnt, queue_family_props);

            for (uint32_t j = 0; j < queue_family_prop_cnt; ++j)
            {
            }
        }

        return false;
    }

    bool RHI::initDevice() {}

    RHI g_rhi;
} // namespace Nano