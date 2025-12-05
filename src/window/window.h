#pragma once

#include <functional>
#include <string>

#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

struct GLFWwindow;

namespace Nano
{
    struct WindowConfig
    {
        int         width {640};
        int         height {480};
        std::string title {"Nano"};
        bool        resizable {true};
    };

    struct GLFWwindowDeleter
    {
        void operator()(GLFWwindow* window) const
        {
            if (window)
                glfwDestroyWindow(window);
        }
    };

    class Window
    {
    public:
        explicit Window(const WindowConfig& config = WindowConfig {});
        virtual ~Window() noexcept = default;

        Window(Window&&) noexcept            = default;
        Window& operator=(Window&&) noexcept = default;
        Window(const Window&)                = delete;
        Window& operator=(const Window&)     = delete;

        virtual bool        shouldClose() const   = 0;
        virtual void        pollEvents() const    = 0;
        virtual GLFWwindow* getGLFWWindow() const = 0;

        int         getWidth() const { return m_width; }
        int         getHeight() const { return m_height; }
        const char* getTitle() const { return m_title.c_str(); }

        // Event callbacks
        using OnResetFunc           = std::function<void()>;
        using OnKeyFunc             = std::function<void(int, int, int, int)>;
        using OnCharFunc            = std::function<void(unsigned int)>;
        using OnCharModsFunc        = std::function<void(int, unsigned int)>;
        using OnMouseButtonFunc     = std::function<void(int, int, int)>;
        using OnCursorPosFunc       = std::function<void(double, double)>;
        using OnCursorEnterFunc     = std::function<void(int)>;
        using OnScrollFunc          = std::function<void(double, double)>;
        using OnDropFunc            = std::function<void(int, const char**)>;
        using OnWindowSizeFunc      = std::function<void(int, int)>;
        using OnFramebufferSizeFunc = std::function<void(int, int)>;
        using OnWindowCloseFunc     = std::function<void()>;

        virtual void registerOnResetFunc(OnResetFunc func)                     = 0;
        virtual void registerOnKeyFunc(OnKeyFunc func)                         = 0;
        virtual void registerOnCharFunc(OnCharFunc func)                       = 0;
        virtual void registerOnCharModsFunc(OnCharModsFunc func)               = 0;
        virtual void registerOnMouseButtonFunc(OnMouseButtonFunc func)         = 0;
        virtual void registerOnCursorPosFunc(OnCursorPosFunc func)             = 0;
        virtual void registerOnCursorEnterFunc(OnCursorEnterFunc func)         = 0;
        virtual void registerOnScrollFunc(OnScrollFunc func)                   = 0;
        virtual void registerOnDropFunc(OnDropFunc func)                       = 0;
        virtual void registerOnWindowSizeFunc(OnWindowSizeFunc func)           = 0;
        virtual void registerOnFramebufferSizeFunc(OnFramebufferSizeFunc func) = 0;
        virtual void registerOnWindowCloseFunc(OnWindowCloseFunc func)         = 0;

    protected:
        int         m_width;
        int         m_height;
        std::string m_title;
        bool        m_resizable;
    };

} // namespace Nano
