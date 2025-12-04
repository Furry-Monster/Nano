#include "vulkan_instance.h"
#include <GLFW/glfw3.h>
#include <cstring>
#include <iostream>
#include <stdexcept>

namespace Nano
{
    VulkanInstance::~VulkanInstance() noexcept { clean(); }

    VulkanInstance::VulkanInstance(VulkanInstance&& other) noexcept :
        m_instance(other.m_instance), m_debug_messenger(other.m_debug_messenger),
        m_enable_validation_layers(other.m_enable_validation_layers)
    {
        other.m_instance                 = VK_NULL_HANDLE;
        other.m_debug_messenger          = VK_NULL_HANDLE;
        other.m_enable_validation_layers = false;
    }

    VulkanInstance& VulkanInstance::operator=(VulkanInstance&& other) noexcept
    {
        if (this != &other)
        {
            clean();
            m_instance                 = other.m_instance;
            m_debug_messenger          = other.m_debug_messenger;
            m_enable_validation_layers = other.m_enable_validation_layers;

            other.m_instance                 = VK_NULL_HANDLE;
            other.m_debug_messenger          = VK_NULL_HANDLE;
            other.m_enable_validation_layers = false;
        }
        return *this;
    }

    void VulkanInstance::init(const std::string& appName, uint32_t appVersion, bool enableValidationLayers)
    {
        m_enable_validation_layers = enableValidationLayers;
        createInstance(appName, appVersion);
        if (m_enable_validation_layers)
        {
            setupDebugMessenger();
        }
    }

    void VulkanInstance::clean()
    {
        if (m_debug_messenger != VK_NULL_HANDLE)
        {
            auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
            if (func != nullptr)
            {
                func(m_instance, m_debug_messenger, nullptr);
            }
            m_debug_messenger = VK_NULL_HANDLE;
        }

        if (m_instance != VK_NULL_HANDLE)
        {
            vkDestroyInstance(m_instance, nullptr);
            m_instance = VK_NULL_HANDLE;
        }
    }

    VkSurfaceKHR VulkanInstance::createSurface(GLFWwindow* window)
    {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(m_instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create window surface!");
        }
        return surface;
    }

    void VulkanInstance::createInstance(const std::string& appName, uint32_t appVersion)
    {
        if (m_enable_validation_layers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("Validation layers requested, but not available!");
        }

        VkApplicationInfo app_info {};
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName   = appName.c_str();
        app_info.applicationVersion = appVersion;
        app_info.pEngineName        = "Nano Engine";
        app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion         = VK_API_VERSION_1_3;

        VkInstanceCreateInfo create_info {};
        create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        auto extensions                     = getRequiredExtensions();
        create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info {};
        if (m_enable_validation_layers)
        {
            create_info.enabledLayerCount   = static_cast<uint32_t>(m_validation_layers.size());
            create_info.ppEnabledLayerNames = m_validation_layers.data();
            populateDebugMessengerCreateInfo(debug_create_info);
            create_info.pNext = &debug_create_info;
        }
        else
        {
            create_info.enabledLayerCount = 0;
            create_info.pNext             = nullptr;
        }

        if (vkCreateInstance(&create_info, nullptr, &m_instance) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create Vulkan instance!");
        }
    }

    void VulkanInstance::setupDebugMessenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT create_info;
        populateDebugMessengerCreateInfo(create_info);

        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func != nullptr)
        {
            if (func(m_instance, &create_info, nullptr, &m_debug_messenger) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to set up debug messenger!");
            }
        }
        else
        {
            throw std::runtime_error("Failed to load vkCreateDebugUtilsMessengerEXT!");
        }
    }

    bool VulkanInstance::checkValidationLayerSupport() const
    {
        uint32_t layer_count;
        vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

        std::vector<VkLayerProperties> available_layers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

        for (const char* layer_name : m_validation_layers)
        {
            bool layer_found = false;

            for (const auto& layer_properties : available_layers)
            {
                if (strcmp(layer_name, layer_properties.layerName) == 0)
                {
                    layer_found = true;
                    break;
                }
            }

            if (!layer_found)
            {
                return false;
            }
        }

        return true;
    }

    std::vector<const char*> VulkanInstance::getRequiredExtensions() const
    {
        uint32_t     glfw_extension_count = 0;
        const char** glfw_extensions;
        glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

        std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

        if (m_enable_validation_layers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    void VulkanInstance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const
    {
        createInfo                 = {};
        createInfo.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL
    VulkanInstance::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
                                  VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                                  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                  void* /*pUserData*/)
    {
        std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

} // namespace Nano
