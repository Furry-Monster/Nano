#include "opengl_window.h"
#include <stdexcept>
#include "GLFW/glfw3.h"

namespace Nano
{
    OpenGLWindow::OpenGLWindow(const OpenGLWindowConfig& gl_config) : Window(gl_config)
    {
        m_msaa_samples = gl_config.msaa_samples;
        m_vsync        = gl_config.vsync;

        // init window handle
        if (glfwInit() == GLFW_FALSE)
            throw std::runtime_error("Failed to initialize GLFW.");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, m_resizable ? GLFW_TRUE : GLFW_FALSE);

        if (m_msaa_samples > 0)
            glfwWindowHint(GLFW_SAMPLES, m_msaa_samples);

        auto* glfw_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
        if (!glfw_window)
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window.");
        }
        glfwMakeContextCurrent(glfw_window);
        glfwSetWindowUserPointer(glfw_window, this);

        if (!gladLoadGL(glfwGetProcAddress))
        {
            glfwDestroyWindow(glfw_window);
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLAD.");
        }

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

        glViewport(0, 0, m_width, m_height);
        glfwSwapInterval(m_vsync ? 1 : 0);
        if (m_msaa_samples > 0)
            glEnable(GL_MULTISAMPLE);

        m_window.reset(glfw_window);
    }

    OpenGLWindow::~OpenGLWindow() noexcept
    {
        if (m_window != nullptr)
            m_window.reset();

        glfwTerminate();
    }

    bool OpenGLWindow::shouldClose() const { return glfwWindowShouldClose(m_window.get()); }

    void OpenGLWindow::pollEvents() const { glfwPollEvents(); }

    GLFWwindow* OpenGLWindow::getGLFWWindow() const { return m_window.get(); }

    void OpenGLWindow::swapBuffer() const { glfwSwapBuffers(m_window.get()); }

    int OpenGLWindow::getMSAASamples() const { return m_msaa_samples; }

    bool OpenGLWindow::isMSAAEnabled() const { return m_msaa_samples > 0; }

    bool OpenGLWindow::isVSyncEnabled() const { return m_vsync; }

    void OpenGLWindow::setCursorMode(int mode) const { glfwSetInputMode(m_window.get(), GLFW_CURSOR, mode); }

    void OpenGLWindow::registerOnResetFunc(OnResetFunc func) { m_on_reset_func.push_back(func); }

    void OpenGLWindow::registerOnKeyFunc(OnKeyFunc func) { m_on_key_func.push_back(func); }

    void OpenGLWindow::registerOnCharFunc(OnCharFunc func) { m_on_char_func.push_back(func); }

    void OpenGLWindow::registerOnCharModsFunc(OnCharModsFunc func) { m_on_char_mods_func.push_back(func); }

    void OpenGLWindow::registerOnMouseButtonFunc(OnMouseButtonFunc func) { m_on_mouse_button_func.push_back(func); }

    void OpenGLWindow::registerOnCursorPosFunc(OnCursorPosFunc func) { m_on_cursor_pos_func.push_back(func); }

    void OpenGLWindow::registerOnCursorEnterFunc(OnCursorEnterFunc func) { m_on_cursor_enter_func.push_back(func); }

    void OpenGLWindow::registerOnScrollFunc(OnScrollFunc func) { m_on_scroll_func.push_back(func); }

    void OpenGLWindow::registerOnDropFunc(OnDropFunc func) { m_on_drop_func.push_back(func); }

    void OpenGLWindow::registerOnWindowSizeFunc(OnWindowSizeFunc func) { m_on_window_size_func.push_back(func); }

    void OpenGLWindow::registerOnFramebufferSizeFunc(OnFramebufferSizeFunc func)
    {
        m_on_framebuffer_size_func.push_back(func);
    }

    void OpenGLWindow::registerOnWindowCloseFunc(OnWindowCloseFunc func) { m_on_window_close_func.push_back(func); }

    // Static callback implementations
    void OpenGLWindow::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onKey(key, scancode, action, mods);
        }
    }

    void OpenGLWindow::charCallback(GLFWwindow* window, unsigned int codepoint)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onChar(codepoint);
        }
    }

    void OpenGLWindow::charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onCharMods(codepoint, mods);
        }
    }

    void OpenGLWindow::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onMouseButton(button, action, mods);
        }
    }

    void OpenGLWindow::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onCursorPos(xpos, ypos);
        }
    }

    void OpenGLWindow::cursorEnterCallback(GLFWwindow* window, int entered)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onCursorEnter(entered);
        }
    }

    void OpenGLWindow::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onScroll(xoffset, yoffset);
        }
    }

    void OpenGLWindow::dropCallback(GLFWwindow* window, int count, const char** paths)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onDrop(count, paths);
        }
    }

    void OpenGLWindow::windowSizeCallback(GLFWwindow* window, int width, int height)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onWindowSize(width, height);
            app->m_width  = width;
            app->m_height = height;
        }
    }

    void OpenGLWindow::framebufferSizeCallback(GLFWwindow* window, int width, int height)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onFramebufferSize(width, height);
        }
    }

    void OpenGLWindow::windowCloseCallback(GLFWwindow* window)
    {
        OpenGLWindow* app = reinterpret_cast<OpenGLWindow*>(glfwGetWindowUserPointer(window));
        if (app)
        {
            app->onWindowClose();
            glfwSetWindowShouldClose(window, true);
        }
    }

    // Event execution methods
    void OpenGLWindow::onReset()
    {
        for (auto& func : m_on_reset_func)
            func();
    }

    void OpenGLWindow::onKey(int key, int scancode, int action, int mods)
    {
        for (auto& func : m_on_key_func)
            func(key, scancode, action, mods);
    }

    void OpenGLWindow::onChar(unsigned int codepoint)
    {
        for (auto& func : m_on_char_func)
            func(codepoint);
    }

    void OpenGLWindow::onCharMods(int codepoint, unsigned int mods)
    {
        for (auto& func : m_on_char_mods_func)
            func(codepoint, mods);
    }

    void OpenGLWindow::onMouseButton(int button, int action, int mods)
    {
        for (auto& func : m_on_mouse_button_func)
            func(button, action, mods);
    }

    void OpenGLWindow::onCursorPos(double xpos, double ypos)
    {
        for (auto& func : m_on_cursor_pos_func)
            func(xpos, ypos);
    }

    void OpenGLWindow::onCursorEnter(int entered)
    {
        for (auto& func : m_on_cursor_enter_func)
            func(entered);
    }

    void OpenGLWindow::onScroll(double xoffset, double yoffset)
    {
        for (auto& func : m_on_scroll_func)
            func(xoffset, yoffset);
    }

    void OpenGLWindow::onDrop(int count, const char** paths)
    {
        for (auto& func : m_on_drop_func)
            func(count, paths);
    }

    void OpenGLWindow::onWindowSize(int width, int height)
    {
        for (auto& func : m_on_window_size_func)
            func(width, height);
    }

    void OpenGLWindow::onFramebufferSize(int width, int height)
    {
        for (auto& func : m_on_framebuffer_size_func)
            func(width, height);
    }

    void OpenGLWindow::onWindowClose()
    {
        for (auto& func : m_on_window_close_func)
            func();
    }

} // namespace Nano
