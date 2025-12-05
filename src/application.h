#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include "misc/logger.h"
#include "renderer/renderer.h"
#include "window/window.h"

namespace Nano
{
    class OpenGLRenderer;
    class VulkanRenderer;

    enum class GraphicsAPI : uint8_t
    {
        Vulkan,
        OpenGL,
    };

    struct ApplicationConfig
    {
        GraphicsAPI graphics_api {GraphicsAPI::Vulkan};

        WindowConfig window_config {};
        LoggerConfig logger_config {};
    };

    class Application
    {
    public:
        explicit Application(const ApplicationConfig& config = {});
        ~Application() noexcept;

        Application(Application&&) noexcept            = default;
        Application& operator=(Application&&) noexcept = default;
        Application(const Application&)                = delete;
        Application& operator=(const Application&)     = delete;

        void run();

    private:
        double timeTick();
        void   logicalTick(double delta_time);
        void   renderTick(double delta_time);

        std::chrono::high_resolution_clock::time_point m_start_time;
        std::chrono::high_resolution_clock::time_point m_previous_tick_time;

        ApplicationConfig m_config;
        GraphicsAPI       m_graphics_api;

        std::shared_ptr<Logger>   m_logger {nullptr};
        std::shared_ptr<Window>   m_window {nullptr};
        std::shared_ptr<Renderer> m_renderer {nullptr};
    };
} // namespace Nano