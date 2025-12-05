#pragma once

#include <memory>
#include <vector>
#include "core/camera.h"
#include "light.h"
#include "pbr_framebuffer.h"
#include "renderer/renderer.h"
#include "shader.h"

namespace Nano
{
    class OpenGLRenderer final : public Renderer
    {
    public:
        explicit OpenGLRenderer(std::shared_ptr<Window> window);
        ~OpenGLRenderer() noexcept override;

        OpenGLRenderer(OpenGLRenderer&&) noexcept            = default;
        OpenGLRenderer& operator=(OpenGLRenderer&&) noexcept = default;
        OpenGLRenderer(const OpenGLRenderer&)                = delete;
        OpenGLRenderer& operator=(const OpenGLRenderer&)     = delete;

        void beginFrame() override;
        void endFrame() override;

        void addLight(const Light& light);
        void clearLights();

        Camera&       getCamera() { return m_camera; }
        const Camera& getCamera() const { return m_camera; }

    private:
        std::unique_ptr<PBRFramebuffer> m_pbr_framebuffer;
        std::unique_ptr<Shader>         m_pbr_shader;
        std::unique_ptr<LightUBO>       m_light_ubo;

        Camera             m_camera;
        std::vector<Light> m_lights;
        glm::vec3          m_ambient_color {0.1f, 0.1f, 0.1f};
    };

} // namespace Nano
