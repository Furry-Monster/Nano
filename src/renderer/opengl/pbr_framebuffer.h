#pragma once

namespace Nano
{
    class PBRFramebuffer
    {
    public:
        PBRFramebuffer(int width, int height);
        ~PBRFramebuffer() noexcept;

        PBRFramebuffer(const PBRFramebuffer&)            = delete;
        PBRFramebuffer& operator=(const PBRFramebuffer&) = delete;
        PBRFramebuffer(PBRFramebuffer&&) noexcept;
        PBRFramebuffer& operator=(PBRFramebuffer&&) noexcept;

        void bind() const;
        void resize(int width, int height);

        unsigned int getFramebufferId() const { return m_framebuffer; }
        unsigned int getColorTextureId() const { return m_color_texture; }

    private:
        int          m_width, m_height;
        unsigned int m_framebuffer;
        unsigned int m_color_texture;
        unsigned int m_depth_stencil_renderbuffer;
    };

} // namespace Nano
