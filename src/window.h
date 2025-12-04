#pragma once

#include <functional>
#include <memory>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Nano
{
    struct GLFWWindowDeleter
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
        Window() = default;
        Window(int width, int height);
        ~Window() noexcept;

        Window(Window&&) noexcept            = default;
        Window& operator=(Window&&) noexcept = default;
        Window(const Window&)                = delete;
        Window& operator=(const Window&)     = delete;

        void init();
        void clean();

        bool supportVulkan() const { return glfwVulkanSupported() == GLFW_TRUE; }
        bool shouldClose() const { return glfwWindowShouldClose(m_window.get()); }
        void pollEvents() const { glfwPollEvents(); }

        int         getWidth() const { return m_width; }
        int         getHeight() const { return m_height; }
        const char* getTitle() const { return m_title; }
        GLFWwindow* getGLFWWindow() const { return m_window.get(); }

        using onResetFunc           = std::function<void()>;
        using onKeyFunc             = std::function<void(int, int, int, int)>;
        using onCharFunc            = std::function<void(unsigned int)>;
        using onCharModsFunc        = std::function<void(int, unsigned int)>;
        using onMouseButtonFunc     = std::function<void(int, int, int)>;
        using onCursorPosFunc       = std::function<void(double, double)>;
        using onCursorEnterFunc     = std::function<void(int)>;
        using onScrollFunc          = std::function<void(double, double)>;
        using onDropFunc            = std::function<void(int, const char**)>;
        using onWindowSizeFunc      = std::function<void(int, int)>;
        using onFramebufferSizeFunc = std::function<void(int, int)>;
        using onWindowCloseFunc     = std::function<void()>;

        void registerOnResetFunc(onResetFunc func) { m_on_reset_func.push_back(func); }
        void registerOnKeyFunc(onKeyFunc func) { m_on_key_func.push_back(func); }
        void registerOnCharFunc(onCharFunc func) { m_on_char_func.push_back(func); }
        void registerOnCharModsFunc(onCharModsFunc func) { m_on_char_mods_func.push_back(func); }
        void registerOnMouseButtonFunc(onMouseButtonFunc func) { m_on_mouse_button_func.push_back(func); }
        void registerOnCursorPosFunc(onCursorPosFunc func) { m_on_cursor_pos_func.push_back(func); }
        void registerOnCursorEnterFunc(onCursorEnterFunc func) { m_on_cursor_enter_func.push_back(func); }
        void registerOnScrollFunc(onScrollFunc func) { m_on_scroll_func.push_back(func); }
        void registerOnDropFunc(onDropFunc func) { m_on_drop_func.push_back(func); }
        void registerOnWindowSizeFunc(onWindowSizeFunc func) { m_on_window_size_func.push_back(func); }
        void registerOnFramebufferSizeFunc(onFramebufferSizeFunc func) { m_on_framebuffer_size_func.push_back(func); }
        void registerOnWindowCloseFunc(onWindowCloseFunc func) { m_on_window_close_func.push_back(func); }

    protected:
        // event handler
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onKey(key, scancode, action, mods);
            }
        }
        static void charCallback(GLFWwindow* window, unsigned int codepoint)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onChar(codepoint);
            }
        }
        static void charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onCharMods(codepoint, mods);
            }
        }
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onMouseButton(button, action, mods);
            }
        }
        static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onCursorPos(xpos, ypos);
            }
        }
        static void cursorEnterCallback(GLFWwindow* window, int entered)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onCursorEnter(entered);
            }
        }
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onScroll(xoffset, yoffset);
            }
        }
        static void dropCallback(GLFWwindow* window, int count, const char** paths)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onDrop(count, paths);
            }
        }
        static void windowSizeCallback(GLFWwindow* window, int width, int height)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onWindowSize(width, height);
                app->m_width  = width;
                app->m_height = height;
            }
        }
        static void framebufferSizeCallback(GLFWwindow* window, int width, int height)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onFramebufferSize(width, height);
            }
        }
        static void windowCloseCallback(GLFWwindow* window)
        {
            Window* app = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (app)
            {
                app->onWindowClose();
                glfwSetWindowShouldClose(window, true);
            }
        }

        // event exec
        void onReset()
        {
            for (auto& func : m_on_reset_func)
                func();
        }
        void onKey(int key, int scancode, int action, int mods)
        {
            for (auto& func : m_on_key_func)
                func(key, scancode, action, mods);
        }
        void onChar(unsigned int codepoint)
        {
            for (auto& func : m_on_char_func)
                func(codepoint);
        }
        void onCharMods(int codepoint, unsigned int mods)
        {
            for (auto& func : m_on_char_mods_func)
                func(codepoint, mods);
        }
        void onMouseButton(int button, int action, int mods)
        {
            for (auto& func : m_on_mouse_button_func)
                func(button, action, mods);
        }
        void onCursorPos(double xpos, double ypos)
        {
            for (auto& func : m_on_cursor_pos_func)
                func(xpos, ypos);
        }
        void onCursorEnter(int entered)
        {
            for (auto& func : m_on_cursor_enter_func)
                func(entered);
        }
        void onScroll(double xoffset, double yoffset)
        {
            for (auto& func : m_on_scroll_func)
                func(xoffset, yoffset);
        }
        void onDrop(int count, const char** paths)
        {
            for (auto& func : m_on_drop_func)
                func(count, paths);
        }
        void onWindowSize(int width, int height)
        {
            for (auto& func : m_on_window_size_func)
                func(width, height);
        }
        void onFramebufferSize(int width, int height)
        {
            for (auto& func : m_on_framebuffer_size_func)
                func(width, height);
        }
        void onWindowClose()
        {
            for (auto& func : m_on_window_close_func)
                func();
        }

    private:
        std::unique_ptr<GLFWwindow, GLFWWindowDeleter> m_window {nullptr};
        int                                            m_width {640};
        int                                            m_height {480};
        const char*                                    m_title {"Nano\0"};

        // events
        std::vector<onResetFunc>           m_on_reset_func;
        std::vector<onKeyFunc>             m_on_key_func;
        std::vector<onCharFunc>            m_on_char_func;
        std::vector<onCharModsFunc>        m_on_char_mods_func;
        std::vector<onMouseButtonFunc>     m_on_mouse_button_func;
        std::vector<onCursorPosFunc>       m_on_cursor_pos_func;
        std::vector<onCursorEnterFunc>     m_on_cursor_enter_func;
        std::vector<onScrollFunc>          m_on_scroll_func;
        std::vector<onDropFunc>            m_on_drop_func;
        std::vector<onWindowSizeFunc>      m_on_window_size_func;
        std::vector<onFramebufferSizeFunc> m_on_framebuffer_size_func;
        std::vector<onWindowCloseFunc>     m_on_window_close_func;
    };
} // namespace Nano