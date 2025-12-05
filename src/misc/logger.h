#pragma once

#include <spdlog/logger.h>
#include <cstdint>
#include <memory>
#include <string>

namespace Nano
{
    class Logger;

    enum class LogLevel : uint8_t
    {
        debug,
        info,
        warn,
        error,
        fatal,
    };

    struct LoggerConfig
    {
        bool        enable_console {true};
        bool        enable_file {false};
        std::string log_file_path {"logs/nano.log"};
        uint32_t    max_file_size_mb {10};
        uint32_t    max_files {5};
        LogLevel    log_level {LogLevel::debug};
    };

    class Logger final
    {
    public:
        explicit Logger(const LoggerConfig& config = {});
        ~Logger() noexcept;

        Logger(const Logger& that)                = delete;
        Logger& operator=(const Logger& that)     = delete;
        Logger(Logger&& that) noexcept            = default;
        Logger& operator=(Logger&& that) noexcept = default;

        template<typename... TARGS>
        void log(LogLevel level, TARGS&&... args) const
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
        static constexpr spdlog::level::level_enum convertLogLevel(LogLevel level);

        std::shared_ptr<spdlog::logger> m_spd_logger;
    };
} // namespace Nano
