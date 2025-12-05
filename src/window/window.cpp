#include "window.h"

namespace Nano
{
    Window::Window(const WindowConfig& config) :
        m_width(config.width), m_height(config.height), m_title(config.title), m_resizable(config.resizable)
    {}

} // namespace Nano
