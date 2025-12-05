#include "logger.h"
#include <exception>
#include <memory>
#include <vector>
#include "spdlog/async.h"
#include "spdlog/async_logger.h"
#include "spdlog/common.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace Nano
{
    constexpr spdlog::level::level_enum Logger::convertLogLevel(LogLevel level)
    {
        switch (level)
        {
            case LogLevel::debug:
                return spdlog::level::debug;
            case LogLevel::info:
                return spdlog::level::info;
            case LogLevel::warn:
                return spdlog::level::warn;
            case LogLevel::error:
                return spdlog::level::err;
            case LogLevel::fatal:
                return spdlog::level::critical;
            default:
                return spdlog::level::debug;
        }
    }

    Logger::Logger(const LoggerConfig& config)
    {
        std::vector<spdlog::sink_ptr> sinks;
        auto                          spdlog_level = convertLogLevel(config.log_level);

        if (config.enable_console)
        {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog_level);
            console_sink->set_pattern("[%^%l%$] %v");
            sinks.push_back(console_sink);
        }

        if (config.enable_file)
        {
            try
            {
                // rotate file will create new file if lack in mem
                auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                    config.log_file_path, config.max_file_size_mb * 1024 * 1024, config.max_files);
                file_sink->set_level(spdlog_level);
                file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
                sinks.push_back(file_sink);
            }
            catch (const std::exception& e)
            {}
        }

        // at least there should be one output if logger enabled (aka. not in Release Building)
        if (sinks.empty())
        {
            auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
            console_sink->set_level(spdlog_level);
            console_sink->set_pattern("[%^%l%$] %v");
            sinks.push_back(console_sink);
        }

        spdlog::init_thread_pool(8192, 1);
        m_spd_logger = std::make_shared<spdlog::async_logger>(
            "nano_logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

        m_spd_logger->set_level(spdlog_level);
        spdlog::register_logger(m_spd_logger);

        log(LogLevel::info,
            "[{}] Logger initialized. Console: {}, File: {}, Level: {}",
            __FUNCTION__,
            config.enable_console,
            config.enable_file,
            static_cast<int>(config.log_level));
    }

    Logger::~Logger() noexcept
    {
        log(LogLevel::info, "[{}] Stop logging and saving...", __FUNCTION__);

        m_spd_logger->flush();
        spdlog::drop_all();
        m_spd_logger.reset();
    }

} // namespace Nano
