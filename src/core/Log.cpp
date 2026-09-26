#include "core/Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

void Log::Init() {
    spdlog::set_default_logger(spdlog::stdout_color_mt("Alpha3D"));
    spdlog::set_pattern("%^[%T] %v%$");
    spdlog::set_level(spdlog::level::trace);
}

void Log::Shutdown() {
    spdlog::shutdown();
}
