#ifndef RHI_H
#define RHI_H

#include <vulkan/vulkan_core.h>
#include <cstdint>

namespace Nano
{
    class RHI final
    {
    public:
        RHI();
        ~RHI() noexcept;

        RHI(const RHI&)            = delete;
        RHI& operator=(const RHI&) = delete;
        RHI(RHI&&)                 = delete;
        RHI& operator=(RHI&&)      = delete;

        VkDevice getDevice() const;
        VkQueue  getQueue() const;

        void createBuffer();
        void createTexture();

        VkInstance   m_instance {VK_NULL_HANDLE};
        uint32_t     m_enabled_ext_cnt {0};
        const char** m_enabled_exts {nullptr};
        uint32_t     m_prefered_layer_cnt {0};
        char**       m_prefered_layers {nullptr};

        VkDebugReportCallbackEXT m_debug_report_callback;

        VkSurfaceKHR     m_surface {VK_NULL_HANDLE};
        VkDevice         m_device {VK_NULL_HANDLE};
        VkPhysicalDevice m_physical_device {VK_NULL_HANDLE};
        VkQueue          m_graphic_queue {VK_NULL_HANDLE};
        VkQueue          m_present_queue {VK_NULL_HANDLE};

    private:
        bool initInstance();
        bool initDebugger();
        bool initSurface();
        bool initPhysicalDevice();
        bool initDevice();
    };

    extern RHI g_rhi;

} // namespace Nano

#endif // !RHI_H