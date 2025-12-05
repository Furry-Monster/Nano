#pragma once

#include <memory>
#include <vector>
#include "window/window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Nano
{
    struct VulkanWindowConfig : WindowConfig
    {};

    class VulkanWindow final : public Window
    {
    public:
        explicit VulkanWindow(const VulkanWindowConfig& config = {});
        virtual ~VulkanWindow() noexcept override;

        VulkanWindow(VulkanWindow&&) noexcept            = default;
        VulkanWindow& operator=(VulkanWindow&&) noexcept = default;
        VulkanWindow(const VulkanWindow&)                = delete;
        VulkanWindow& operator=(const VulkanWindow&)     = delete;

        bool        shouldClose() const override;
        void        pollEvents() const override;
        GLFWwindow* getGLFWWindow() const override;

        bool supportVulkan() const;

        // Event callbacks
        void registerOnResetFunc(OnResetFunc func) override;
        void registerOnKeyFunc(OnKeyFunc func) override;
        void registerOnCharFunc(OnCharFunc func) override;
        void registerOnCharModsFunc(OnCharModsFunc func) override;
        void registerOnMouseButtonFunc(OnMouseButtonFunc func) override;
        void registerOnCursorPosFunc(OnCursorPosFunc func) override;
        void registerOnCursorEnterFunc(OnCursorEnterFunc func) override;
        void registerOnScrollFunc(OnScrollFunc func) override;
        void registerOnDropFunc(OnDropFunc func) override;
        void registerOnWindowSizeFunc(OnWindowSizeFunc func) override;
        void registerOnFramebufferSizeFunc(OnFramebufferSizeFunc func) override;
        void registerOnWindowCloseFunc(OnWindowCloseFunc func) override;

    protected:
        // event handler
        static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void charCallback(GLFWwindow* window, unsigned int codepoint);
        static void charModsCallback(GLFWwindow* window, unsigned int codepoint, int mods);
        static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
        static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos);
        static void cursorEnterCallback(GLFWwindow* window, int entered);
        static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
        static void dropCallback(GLFWwindow* window, int count, const char** paths);
        static void windowSizeCallback(GLFWwindow* window, int width, int height);
        static void framebufferSizeCallback(GLFWwindow* window, int width, int height);
        static void windowCloseCallback(GLFWwindow* window);

        // event exec
        void onReset();
        void onKey(int key, int scancode, int action, int mods);
        void onChar(unsigned int codepoint);
        void onCharMods(int codepoint, unsigned int mods);
        void onMouseButton(int button, int action, int mods);
        void onCursorPos(double xpos, double ypos);
        void onCursorEnter(int entered);
        void onScroll(double xoffset, double yoffset);
        void onDrop(int count, const char** paths);
        void onWindowSize(int width, int height);
        void onFramebufferSize(int width, int height);
        void onWindowClose();

    private:
        std::unique_ptr<GLFWwindow, GLFWwindowDeleter> m_window {nullptr};

        // events
        std::vector<OnResetFunc>           m_on_reset_func;
        std::vector<OnKeyFunc>             m_on_key_func;
        std::vector<OnCharFunc>            m_on_char_func;
        std::vector<OnCharModsFunc>        m_on_char_mods_func;
        std::vector<OnMouseButtonFunc>     m_on_mouse_button_func;
        std::vector<OnCursorPosFunc>       m_on_cursor_pos_func;
        std::vector<OnCursorEnterFunc>     m_on_cursor_enter_func;
        std::vector<OnScrollFunc>          m_on_scroll_func;
        std::vector<OnDropFunc>            m_on_drop_func;
        std::vector<OnWindowSizeFunc>      m_on_window_size_func;
        std::vector<OnFramebufferSizeFunc> m_on_framebuffer_size_func;
        std::vector<OnWindowCloseFunc>     m_on_window_close_func;
    };

} // namespace Nano
