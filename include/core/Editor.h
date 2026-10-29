#pragma once
#include <vector>
#include <string>
#include <utility>

#include <imgui.h>

class GameObject;
class Window;
class Camera;
class Scene;
class IBL;
class SkyAtmosphere;
class SunLight;
class VolumetricCloud;
class RenderProfiler;
class Terrain;
class PostProcess;
class Capture;

#include "renderer/Model.h"
#include "renderer/Shader.h"

struct MaterialPreview {
    unsigned int FBO = 0;
    unsigned int texture = 0;
    unsigned int RBO = 0;

    GLsizei width = 320;
    GLsizei height = 320;

    float metallic = 1.0f;
    float roughness = 0.2f;

    Model sphereModel = Model::Sphere();

    Shader shader{ "shaders/pbr/pbr_ibl_test.vert", "shaders/pbr/pbr_ibl_test.frag" };
};

class Editor {
public:

    void BeginTerrain(Terrain& terrain);

    void Init(Window* window);
    void Update(const Window& window);
    ~Editor();

    void BeginFrame();
    void EndFrame();
    void BeginDetails(GameObject& game_object);
    void BeginHierarchy(Scene& scene);
    std::string GetFont() { return font_name; }
    bool Hover() const;
    bool WantCaptureKeyboard() const;
    void BeginCamera(Camera& camera);
    bool BeginSunLight(SunLight& sunLight);
    bool BeginSkyAtmosphere(SkyAtmosphere& sky);
    bool BeginVolumetricCloud(VolumetricCloud& cloud);
    void BeginIBL(IBL& ibl);
    void BeginPostProcess(PostProcess& postProcess);
    void BeginCapture(Capture& capture);
    void BeginPerformance(RenderProfiler& profiler);
    bool HasPerformanceAffectingEdit() const;

    void RenderMaterialPreview(const IBL& ibl, Camera& camera);
    void ShowMaterialPreview();
private:

    void InitMaterialPreview();

    float currentScale = 1.0f;

    std::string font_name;
    ImFont* font_small;

    MaterialPreview materialPreview;
};
