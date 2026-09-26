// Log.h
#pragma once
#include "spdlog/spdlog.h"

#define LOG_TRACE(...) spdlog::trace(__VA_ARGS__)
#define LOG_INFO(...) spdlog::info(__VA_ARGS__)
#define LOG_WARN(...) spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...) spdlog::error(__VA_ARGS__)

class Log {
public:
    static void Init();
    static void Shutdown();
};
