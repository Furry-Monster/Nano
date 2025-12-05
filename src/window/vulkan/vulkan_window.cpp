#include "vulkan_window.h"
#include <stdexcept>

namespace Nano
{
    VulkanWindow::VulkanWindow(const WindowConfig& vk_config) : Window(vk_config)
    {
        // init window handle
        if (glfwInit() == GLFW_FALSE)
            throw std::runtime_error("Failed to initialize GLFW.");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, m_resizable ? GLFW_TRUE : GLFW_FALSE);

        auto* glfw_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
        if (!glfw_window)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window.");
        }
        glfwSetWindowUserPointer(glfw_window, this);

        // Register callbacks
        glfwSetWindowCloseCallback(glfw_window, windowCloseCallback);
        glfwSetWindowSizeCallback(glfw_window, windowSizeCallback);
        glfwSetFramebufferSizeCallback(glfw_window, framebufferSizeCallback);
        glfwSetKeyCallback(glfw_window, keyCallback);
        glfwSetCharCallback(glfw_window, charCallback);
        glfwSetCharModsCallback(glfw_window, charModsCallback);
        glfwSetMouseButtonCallback(glfw_window, mouseButtonCallback);
        glfwSetCursorPosCallback(glfw_window, cursorPosCallback);
        glfwSetCursorEnterCallback(glfw_window, cursorEnterCallback);
        glfwSetScrollCallback(glfw_window, scrollCallback);
        glfwSetDropCallback(glfw_window, dropCallback);

        m_window.reset(glfw_window);
    }

    VulkanWindow::~VulkanWindow() noexcept
    {
        if (m_window != nullptr)
            m_window.reset();

        glfwTerminate();
    }

    bool VulkanWindow::shouldClose() const { return glfwWindowShouldClose(m_window.get()); }

    void VulkanWindow::pollEvents() const { glfwPollEvents(); }

    GLFWwindow* VulkanWindow::getGLFWWindow() const { return m_window.get(); }

    bool VulkanWindow::supportVulkan() const { return glfwVulkanSupported() == GLFW_TRUE; }

    void VulkanWindow::registerOnResetFunc(OnResetFunc func) { m_on_reset_func.push_back(func); }

    void VulkanWindow::registerOnKeyFunc(OnKeyFunc func) { m_on_key_func.push_back(func); }

    void VulkanWindow::registerOnCharFunc(OnCharFunc func) { m_on_char_func.push_back(func); }

    void VulkanWindow::registerOnCharModsFunc(OnCharModsFunc func) { m_on_char_mods_func.push_back(func); }

    void VulkanWindow::registerOnMouseButtonFunc(OnMouseButtonFunc func) { m_on_mouse_button_func.push_back(func); }

    void VulkanWindow::registerOnCursorPosFunc(OnCursorPosFunc func) { m_on_cursor_pos_func.push_back(func); }

    void VulkanWindow::registerOnCursorEnterFunc(OnCursorEnterFunc func) { m_on_cursor_enter_func.push_back(func); }

    void VulkanWindow::registerOnScrollFunc(OnScrollFunc func) { m_on_scroll_func.push_back(func); }

    void VulkanWindow::registerOnDropFunc(OnDropFunc func) { m_on_drop_func.push_back(func); }

    void VulkanWindow::registerOnWindowSizeFunc(OnWindowSizeFunc func) { m_on_window_size_func.push_back(func); }

    void VulkanWindow::registerOnFramebufferSizeFunc(OnFramebufferSizeFunc func)
    {
        m_on_framebuffer_size_func.push_back(func);
    }

    void VulkanWindow::registerOnWindowCloseFunc(OnWindowCloseFunc func) { m_on_window_close_func.push_back(func); }

    // Static callback implementations
    void VulkanWindow::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onKey(key, scancode, action, mods);
        }
    }

    void VulkanWindow::charCallback(GLFWwindow* window, unsigned int codepoint)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onChar(codepoint);
        }
    }

    void VulkanWindow::charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onCharMods(codepoint, mods);
        }
    }

    void VulkanWindow::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onMouseButton(button, action, mods);
        }
    }

    void VulkanWindow::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onCursorPos(xpos, ypos);
        }
    }

    void VulkanWindow::cursorEnterCallback(GLFWwindow* window, int entered)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onCursorEnter(entered);
        }
    }

    void VulkanWindow::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onScroll(xoffset, yoffset);
        }
    }

    void VulkanWindow::dropCallback(GLFWwindow* window, int count, const char** paths)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onDrop(count, paths);
        }
    }

    void VulkanWindow::windowSizeCallback(GLFWwindow* window, int width, int height)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onWindowSize(width, height);
            app->m_width  = width;
            app->m_height = height;
        }
    }

    void VulkanWindow::framebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onFramebufferSize(width, height);
        }
    }

    void VulkanWindow::windowCloseCallback(GLFWwindow* window)
    {
        VulkanWindow* app = reinterpret_cast<VulkanWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onWindowClose();
            glfwSetWindowShouldClose(window, true);
        }
    }

    // Event execution methods
    void VulkanWindow::onReset()
    {
        for (auto& func : m_on_reset_func)
            func();
    }

    void VulkanWindow::onKey(int key, int scancode, int action, int mods)
    {
        for (auto& func : m_on_key_func)
            func(key, scancode, action, mods);
    }

    void VulkanWindow::onChar(unsigned int codepoint)
    {
        for (auto& func : m_on_char_func)
            func(codepoint);
    }

    void VulkanWindow::onCharMods(int codepoint, unsigned int mods)
    {
        for (auto& func : m_on_char_mods_func)
            func(codepoint, mods);
    }

    void VulkanWindow::onMouseButton(int button, int action, int mods)
    {
        for (auto& func : m_on_mouse_button_func)
            func(button, action, mods);
    }

    void VulkanWindow::onCursorPos(double xpos, double ypos)
    {
        for (auto& func : m_on_cursor_pos_func)
            func(xpos, ypos);
    }

    void VulkanWindow::onCursorEnter(int entered)
    {
        for (auto& func : m_on_cursor_enter_func)
            func(entered);
    }

    void VulkanWindow::onScroll(double xoffset, double yoffset)
    {
        for (auto& func : m_on_scroll_func)
            func(xoffset, yoffset);
    }

    void VulkanWindow::onDrop(int count, const char** paths)
    {
        for (auto& func : m_on_drop_func)
            func(count, paths);
    }

    void VulkanWindow::onWindowSize(int width, int height)
    {
        for (auto& func : m_on_window_size_func)
            func(width, height);
    }

    void VulkanWindow::onFramebufferSize(int width, int height)
    {
        for (auto& func : m_on_framebuffer_size_func)
            func(width, height);
    }

    void VulkanWindow::onWindowClose()
    {
        for (auto& func : m_on_window_close_func)
            func();
    }

} // namespace Nano
