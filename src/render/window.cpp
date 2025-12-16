#include "window.h"
#include "misc/logger.h"

namespace Nano
{
    Window::Window()
    {
        if (!glfwInit())
        {
            ERROR("Failed to initialize GLFW.");
            glfwTerminate();
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        GLFWwindow* glfw_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
        if (!glfw_window)
        {
            ERROR("Failed to create GLFW window.");
            glfwTerminate();
            return;
        }

        glfwSetWindowUserPointer(glfw_window, this);
        glfwSetKeyCallback(glfw_window, keyCallback);
        glfwSetCharCallback(glfw_window, charCallback);
        glfwSetCharCallback(glfw_window, charCallback);
        glfwSetCharModsCallback(glfw_window, charModsCallback);
        glfwSetMouseButtonCallback(glfw_window, mouseButtonCallback);
        glfwSetCursorPosCallback(glfw_window, cursorPosCallback);
        glfwSetCursorEnterCallback(glfw_window, cursorEnterCallback);
        glfwSetScrollCallback(glfw_window, scrollCallback);
        glfwSetDropCallback(glfw_window, dropCallback);
        glfwSetWindowSizeCallback(glfw_window, windowSizeCallback);
        glfwSetFramebufferSizeCallback(glfw_window, framebufferSizeCallback);
        glfwSetWindowCloseCallback(glfw_window, windowCloseCallback);

        m_window.reset(glfw_window);

        INFO("Window initialized.");
    }

    Window::~Window() noexcept
    {
        m_window.reset();
        glfwTerminate();

        INFO("Window cleaned up.");
    }

    bool Window::shouldClose() { return glfwWindowShouldClose(m_window.get()); }

    void Window::pollEvents() { glfwPollEvents(); }

    Window g_window;
} // namespace Nano