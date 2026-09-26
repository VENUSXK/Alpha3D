#pragma once
#include <vector>
#include <string>
#include <utility>

#include <imgui.h>

class GameObject;
class Window;
class Viewport;
class Camera;
class Scene;
class IBL;
class SkyAtmosphere;
class VolumetricCloud;
class RenderProfiler;


class Editor {
public:
    void Init(Window* window);
    void Update(const Window& window);
    ~Editor();

    void BeginFrame(Viewport& viewport);
    void EndFrame();
    void BeginEnvironment(IBL& ibl);
    void BeginDetails(GameObject& game_object);
    void BeginHierarchy(Scene& scene);
    std::string GetFont() { return font_name; }
    bool Hover() const;
    bool WantCaptureKeyboard() const;
    bool IsViewportHovered() { return isViewportHovered; }
    void BeginCamera(Camera& camera);
    void BeginSkyAtmosphere(SkyAtmosphere& sky);
    void BeginVolumetricCloud(VolumetricCloud& cloud);
    void BeginPerformance(RenderProfiler& profiler);
    bool HasPerformanceAffectingEdit() const;
    void ShowToast(const std::string& message, float duration = 1.5f);

private:
    float currentScale = 1.0f;

    float mainMenuBarHeight = 10.0f;
    std::string font_name;
    ImFont* font_small;

    ImVec2 viewportSize;
    bool isViewportHovered = false;
    static ImGuiTextBuffer log;
    std::string toast_message;
    float toast_timer = 0.0f;
    float toast_duration = 0.0f;
    float toast_fade_duration = 0.25f;
};
