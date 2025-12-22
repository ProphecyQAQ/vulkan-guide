#pragma once

#include <spdlog/spdlog.h>

namespace Core {
    class Logger
    {
    public:
        static void init();
        static std::shared_ptr<spdlog::logger>& getLogger() { return logger; }
    private:
        static std::shared_ptr<spdlog::logger> logger;
    };
};

// Log macros
#define LOG_DEBUG(...) ::Core::Logger::getLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...)  ::Core::Logger::getLogger()->info(__VA_ARGS__)
#define LOG_WARN(...)  ::Core::Logger::getLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Core::Logger::getLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::Core::Logger::getLogger()->critical(__VA_ARGS__)