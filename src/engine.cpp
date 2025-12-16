#include "engine.h"

#include <exception>
#include "misc/logger.h"
#include "render/rhi/rhi.h"
#include "render/window.h"

namespace Nano
{
    void Engine::run()
    {
        if (m_is_running)
        {
            WARN("Engine is already running now...");
            return;
        }

        try
        {
            init();
            loop();
        }
        catch (const std::exception& e)
        {
            ERROR("Engine error: " + e.what());
        }

        clean();
    }

    void Engine::loop()
    {
        m_curr_time  = Clock::now();
        m_is_running = true;

        while (!Window::instance().shouldClose())
        {
            // time calc and clamp
            TimePoint                     now_time   = Clock::now();
            std::chrono::duration<double> delta_time = now_time - m_curr_time;
            m_curr_time                              = now_time;

            if (delta_time.count() > MAX_DELTA_TIME_STEP) // avoid skipping ticking
                delta_time = std::chrono::duration<double>(MAX_DELTA_TIME_STEP);

            m_accumulator += delta_time;

            // logic tick
            while (m_accumulator >= MS_PER_UPDATE)
            {
                update(MS_PER_UPDATE.count());
                m_accumulator -= MS_PER_UPDATE;
            }

            // render tick
            double interpolated = m_accumulator / MS_PER_UPDATE;
            render(static_cast<float>(interpolated));
        }

        m_is_running = false;
    }

    void Engine::init()
    {
        if (m_is_running)
            return;

        Window::instance();
        RHI::instance();
    }

    void Engine::update(double deltaTime)
    {
        Window::instance().pollEvents();

        // TODO: other system updates;
    }

    void Engine::render(float interpolation)
    {
        // TODO: draw call here.
    }

    void Engine::clean()
    {
        m_is_running  = false;
        m_accumulator = std::chrono::duration<double>::zero();
    }

} // namespace Nano