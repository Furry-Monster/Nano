#include "window.h"
#include <memory>
#include <stdexcept>
#include "GLFW/glfw3.h"

namespace Nano
{
    Window::Window(int width, int height) : m_width(width), m_height(height) {}

    Window::~Window() noexcept { clean(); }

    void Window::init()
    {
        if (glfwInit() == GLFW_FALSE)
            throw std::runtime_error("Failed to initialize GLFW.");

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        auto* glfw_window = glfwCreateWindow(m_width, m_height, m_title, nullptr, nullptr);
        if (!glfw_window)
        {
            glfwTerminate();
            return;
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

    void Window::clean()
    {
        m_window.reset();
        glfwTerminate();
    }
} // namespace Nano