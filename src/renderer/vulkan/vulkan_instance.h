#pragma once

#include <vulkan/vulkan_core.h>
#include <string>
#include <vector>
#include "GLFW/glfw3.h"

namespace Nano
{
    class Window;

    class VulkanInstance
    {
    public:
        VulkanInstance() = default;
        ~VulkanInstance() noexcept;

        VulkanInstance(VulkanInstance&&) noexcept;
        VulkanInstance& operator=(VulkanInstance&&) noexcept;
        VulkanInstance(const VulkanInstance&)            = delete;
        VulkanInstance& operator=(const VulkanInstance&) = delete;

        void init(const std::string& appName, uint32_t appVersion, bool enableValidationLayers = true);
        void clean();

        VkInstance getHandle() const { return m_instance; }
        bool       isValidationEnabled() const { return m_enable_validation_layers; }

        VkSurfaceKHR createSurface(GLFWwindow* window);

    private:
        void                     createInstance(const std::string& appName, uint32_t appVersion);
        void                     setupDebugMessenger();
        bool                     checkValidationLayerSupport() const;
        std::vector<const char*> getRequiredExtensions() const;
        void                     populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) const;

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                            VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                            void*                                       pUserData);

        VkInstance                     m_instance {VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT       m_debug_messenger {VK_NULL_HANDLE};
        bool                           m_enable_validation_layers {true};
        const std::vector<const char*> m_validation_layers {"VK_LAYER_KHRONOS_validation"};
    };

} // namespace Nano
