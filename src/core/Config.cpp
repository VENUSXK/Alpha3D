#include "toml++/toml.hpp"

#include "core/Config.h"
#include "core/Log.h"

#include <utility>

EngineConfig Config::s_Config;

void Config::Load(const std::string& path) {
    try {
        auto data = toml::parse_file(path);

        s_Config.window.width = data["window"]["width"].value_or(800);
        s_Config.window.height = data["window"]["height"].value_or(800);
        s_Config.window.title = data["window"]["title"].value_or("Engine");

        const auto monitorCenterX = data["window"]["monitorCenterX"].value<int>();
        const auto monitorCenterY = data["window"]["monitorCenterY"].value<int>();
        s_Config.window.hasMonitorCenter = monitorCenterX.has_value() && monitorCenterY.has_value();
        if (s_Config.window.hasMonitorCenter) {
            s_Config.window.monitorCenterX = *monitorCenterX;
            s_Config.window.monitorCenterY = *monitorCenterY;
        }

        s_Config.renderer.fov = data["renderer"]["fov"].value_or(90.0f);
        s_Config.renderer.nearClip = data["renderer"]["nearClip"].value_or(
            data["renderer"]["near"].value_or(0.1f));
        s_Config.renderer.farClip = data["renderer"]["farClip"].value_or(
            data["renderer"]["far"].value_or(100.0f));

        LOG_INFO("Config loaded: {}.", path);
    }
    catch (const toml::parse_error& e) {
        LOG_INFO("Failed to load config: {}.", e.what());
    }
}


void Config::Save(const std::string& path) {
    toml::table doc;
    toml::table window{
        {"width",  s_Config.window.width},
        {"height", s_Config.window.height},
        {"title",  s_Config.window.title},
    };
    if (s_Config.window.hasMonitorCenter) {
        window.insert_or_assign("monitorCenterX", s_Config.window.monitorCenterX);
        window.insert_or_assign("monitorCenterY", s_Config.window.monitorCenterY);
    }
    doc.insert_or_assign("window", std::move(window));
    doc.insert_or_assign("renderer", toml::table{
        {"fov",      s_Config.renderer.fov},
        {"nearClip", s_Config.renderer.nearClip},
        {"farClip",  s_Config.renderer.farClip},
    });
    std::ofstream file(path);
    file << doc;
}
