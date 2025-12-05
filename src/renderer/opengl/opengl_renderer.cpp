#include "opengl_renderer.h"

#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>
#include "window/opengl/opengl_window.h"
#include "window/window.h"

namespace Nano
{
    OpenGLRenderer::OpenGLRenderer(std::shared_ptr<Window> window) : Renderer(window)
    {
        auto* opengl_window = dynamic_cast<OpenGLWindow*>(window.get());
        if (!opengl_window)
            throw std::runtime_error("OpenGLRenderer requires OpenGLWindow");

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        int width  = m_window->getWidth();
        int height = m_window->getHeight();

        m_pbr_framebuffer = std::make_unique<PBRFramebuffer>(width, height);

        // Load shaders
        std::filesystem::path shader_path   = "shaders";
        std::string           vertex_path   = (shader_path / "pbr.vert").string();
        std::string           fragment_path = (shader_path / "pbr.frag").string();

        m_pbr_shader = std::make_unique<Shader>(vertex_path, fragment_path);
        m_pbr_shader->bindUniformBlock("LightBlock", LIGHT_UBO_BINDING_POINT);

        m_light_ubo = std::make_unique<LightUBO>();

        glViewport(0, 0, width, height);
    }

    OpenGLRenderer::~OpenGLRenderer() noexcept
    {
        m_pbr_shader.reset();
        m_pbr_framebuffer.reset();
        m_light_ubo.reset();
    }

    void OpenGLRenderer::beginFrame()
    {
        int width  = m_window->getWidth();
        int height = m_window->getHeight();

        m_pbr_framebuffer->bind();
        glViewport(0, 0, width, height);

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        m_pbr_shader->use();
        m_pbr_shader->setVec3("cameraPosition", m_camera_position);
        m_pbr_shader->setVec3("ambientColor", m_ambient_color);

        // Update light UBO
        m_light_ubo->updateLights(m_lights);
        m_light_ubo->bind(LIGHT_UBO_BINDING_POINT);
    }

    void OpenGLRenderer::endFrame()
    {
        // Blit framebuffer to default framebuffer
        int width  = m_window->getWidth();
        int height = m_window->getHeight();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_pbr_framebuffer->getFramebufferId());
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLRenderer::addLight(const Light& light) { m_lights.push_back(light); }

    void OpenGLRenderer::clearLights() { m_lights.clear(); }

} // namespace Nano
