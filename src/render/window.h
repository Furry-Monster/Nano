#ifndef WINDOW_H
#define WINDOW_H

#include <functional>
#include <memory>
#include <string>
#include <vector>
#define GLFW_INCLUDE_NONE
#include "GLFW/glfw3.h"

namespace Nano
{
    struct GLFWwindowDeleter
    {
        void operator()(GLFWwindow* window) const
        {
            if (window)
                glfwDestroyWindow(window);
        }
    };

    class Window final
    {
    public:
        Window();
        ~Window() noexcept;

        Window(const Window& other)            = delete;
        Window& operator=(const Window& other) = delete;
        Window(Window&& other)                 = delete;
        Window& operator=(Window&& other)      = delete;

        bool shouldClose();
        void pollEvents();

        std::string getTitle() const { return m_title; }
        int         getWidth() const { return m_width; }
        int         getHeight() const { return m_height; }
        GLFWwindow* getGLFWWindow() const { return m_window.get(); }

        using OnResetFunc           = std::function<void()>;
        using OnKeyFunc             = std::function<void(int, int, int, int)>;
        using OnCharFunc            = std::function<void(unsigned int)>;
        using OnCharModsFunc        = std::function<void(int, unsigned int)>;
        using OnMouseButtOnFunc     = std::function<void(int, int, int)>;
        using OnCursorPosFunc       = std::function<void(double, double)>;
        using OnCursorEnterFunc     = std::function<void(int)>;
        using OnScrollFunc          = std::function<void(double, double)>;
        using OnDropFunc            = std::function<void(int, const char**)>;
        using OnWindowSizeFunc      = std::function<void(int, int)>;
        using OnFramebufferSizeFunc = std::function<void(int, int)>;
        using OnWindowCloseFunc     = std::function<void()>;

        void registerOnResetFunc(OnResetFunc func) { m_on_reset_func.push_back(func); }
        void registerOnKeyFunc(OnKeyFunc func) { m_on_key_func.push_back(func); }
        void registerOnCharFunc(OnCharFunc func) { m_on_char_func.push_back(func); }
        void registerOnCharModsFunc(OnCharModsFunc func) { m_on_char_mods_func.push_back(func); }
        void registerOnMouseButtonFunc(OnMouseButtOnFunc func) { m_on_mouse_button_func.push_back(func); }
        void registerOnCursorPosFunc(OnCursorPosFunc func) { m_on_cursor_pos_func.push_back(func); }
        void registerOnCursorEnterFunc(OnCursorEnterFunc func) { m_on_cursor_enter_func.push_back(func); }
        void registerOnScrollFunc(OnScrollFunc func) { m_on_scroll_func.push_back(func); }
        void registerOnDropFunc(OnDropFunc func) { m_on_drop_func.push_back(func); }
        void registerOnWindowSizeFunc(OnWindowSizeFunc func) { m_on_window_size_func.push_back(func); }
        void registerOnFramebufferSizeFunc(OnFramebufferSizeFunc func) { m_on_framebuffer_size_func.push_back(func); }
        void registerOnWindowCloseFunc(OnWindowCloseFunc func) { m_on_window_close_func.push_back(func); }

    protected:
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
        std::unique_ptr<GLFWwindow, GLFWwindowDeleter> m_window {nullptr};
        std::string                                    m_title {"Nano"};
        int                                            m_width {1280}, m_height {720};

        std::vector<OnResetFunc>           m_on_reset_func;
        std::vector<OnKeyFunc>             m_on_key_func;
        std::vector<OnCharFunc>            m_on_char_func;
        std::vector<OnCharModsFunc>        m_on_char_mods_func;
        std::vector<OnMouseButtOnFunc>     m_on_mouse_button_func;
        std::vector<OnCursorPosFunc>       m_on_cursor_pos_func;
        std::vector<OnCursorEnterFunc>     m_on_cursor_enter_func;
        std::vector<OnScrollFunc>          m_on_scroll_func;
        std::vector<OnDropFunc>            m_on_drop_func;
        std::vector<OnWindowSizeFunc>      m_on_window_size_func;
        std::vector<OnFramebufferSizeFunc> m_on_framebuffer_size_func;
        std::vector<OnWindowCloseFunc>     m_on_window_close_func;
    };

    extern Window g_window;

} // namespace Nano

#endif // !WINDOW_H