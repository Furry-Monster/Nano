#include "engine.h"
#include <chrono>
#include <exception>
#include "misc/logger.h"

namespace Nano
{
    void Engine::run()
    {
        if (m_is_running)
        {
            WARN("Engine is already running now...")
            return;
        }

        try
        {
            init();

            m_is_running = true;
            loop();
        }
        catch (const std::exception& e)
        {
            clean();
            ERROR("Engine error: " + e.what());
        }

        clean();
    }

    void Engine::loop()
    {
        m_curr_time = Clock::now();

        while (m_is_running)
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
    }

    void Engine::init()
    {
        if (m_is_running)
            return;
    }

    void Engine::update(double deltaTime) {}

    void Engine::render(float interpolation) {}

    void Engine::clean() {}

} // namespace Nano