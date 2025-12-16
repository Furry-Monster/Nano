#ifndef LOGGER_H
#define LOGGER_H

#include <spdlog/logger.h>
#include <cstdint>
#include <memory>

namespace Nano
{
    class Logger final
    {
    public:
        enum class LogLevel : uint8_t
        {
            debug,
            info,
            warn,
            error,
            fatal,
        };

        Logger();
        ~Logger() noexcept;

        Logger(const Logger& that) noexcept            = delete;
        Logger& operator=(const Logger& that) noexcept = delete;
        Logger(Logger&& that) noexcept                 = default;
        Logger& operator=(Logger&& that) noexcept      = default;

        template<typename... TARGS>
        void log(const LogLevel& level, TARGS&&... args) const
        {
            switch (level)
            {
                case LogLevel::debug:
                    m_spd_logger->debug(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::info:
                    m_spd_logger->info(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::warn:
                    m_spd_logger->warn(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::error:
                    m_spd_logger->error(std::forward<TARGS>(args)...);
                    break;
                case LogLevel::fatal:
                    m_spd_logger->critical(std::forward<TARGS>(args)...);
                    const std::string fmt_str = fmt::format(std::forward<TARGS>(args)...);
                    throw std::runtime_error(fmt_str);
                    break;
            }
        }

    private:
        std::shared_ptr<spdlog::logger> m_spd_logger;
    };

    extern Logger g_logger;

#define LOG_HELPER(LOG_LEVEL, ...) Nano::g_logger.log(LOG_LEVEL, "[" + std::string(__FUNCTION__) + "] " + __VA_ARGS__);
#define DEBUG(...) LOG_HELPER(Nano::Logger::LogLevel::debug, __VA_ARGS__);
#define INFO(...) LOG_HELPER(Nano::Logger::LogLevel::info, __VA_ARGS__);
#define WARN(...) LOG_HELPER(Nano::Logger::LogLevel::warn, __VA_ARGS__);
#define ERROR(...) LOG_HELPER(Nano::Logger::LogLevel::error, __VA_ARGS__);
#define FATAL(...) LOG_HELPER(Nano::Logger::LogLevel::fatal, __VA_ARGS__);

} // namespace Nano

#endif // LOGGER_H
