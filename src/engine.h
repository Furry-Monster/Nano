#ifndef ENGINE_H
#define ENGINE_H

#include <chrono>

namespace Nano
{
    class Engine
    {
    public:
        Engine()           = default;
        ~Engine() noexcept = default;

        Engine(const Engine& other)                = delete;
        Engine& operator=(const Engine& other)     = delete;
        Engine(Engine&& other) noexcept            = delete;
        Engine& operator=(Engine&& other) noexcept = delete;

        void run();

    protected:
        void init();
        void update(double deltaTime);
        void render(float interpolation);
        void clean();

    private:
        void loop();

        static constexpr double                        MAX_DELTA_TIME_STEP {0.25};
        static constexpr double                        PHYSICAL_TICK_RATE {60.0}; // 60 times ticking per second
        static constexpr std::chrono::duration<double> MS_PER_UPDATE {
            std::chrono::duration<double>(1.0 / PHYSICAL_TICK_RATE)};

        using Clock     = std::chrono::high_resolution_clock;
        using TimePoint = Clock::time_point;

        TimePoint                     m_curr_time;
        std::chrono::duration<double> m_accumulator {std::chrono::duration<double>::zero()};

        bool m_is_running {false};
    };
} // namespace Nano

#endif // !ENGINE_H