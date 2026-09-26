#pragma once
#include <string>

struct WindowConfig {
    int width = 800, height = 800;
    int monitorCenterX = 0, monitorCenterY = 0;
    bool hasMonitorCenter = false;
    std::string title = "Engine";
};
struct RendererConfig {
    float fov = 90.0f;
    float nearClip = 0.1f;
    float farClip = 100.0f;
};

struct EngineConfig {
    WindowConfig window;
    RendererConfig renderer;
};

class Config {
public:
    static void Load(const std::string& path);
    static void Save(const std::string& path);

    static EngineConfig& Get() { return s_Config; }
private:
    static EngineConfig s_Config;
};
