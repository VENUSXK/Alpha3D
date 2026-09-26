#pragma once
#include "renderer/Mesh.h"
#include "renderer/Model.h"
#include "renderer/Shader.h"
#include "renderer/Camera.h"
#include "renderer/IBL.h"
#include "scene/Scene.h"
#include "components/BaseScene.h"

class Window;

struct SkyAtmosphere1993Parameters
{
    bool useRayleigh = true;
    bool useMie = true;
    bool useAbsorption = true;
    glm::vec3 lightDirection = glm::normalize(glm::vec3(0.0f, 0.55f, -0.84f));
    float lightIntensity = 40.0f;
    float cameraHeight = 100.0f;
    float planetRadius = 6371000.0f;
    float atmosphereRadius = 6471000.0f;
};

class SkyAtmosphere1993 : public BaseScene
{
public:
    void Load(Window& window);
    void Render(Camera& camera, RenderProfiler& profiler);
    void Unload();
    void RenderEditor(Editor& editor);
    std::string GetName() const { return this->name; }

    GameObject* GetMainGameObject() { return nullptr; }

    // Shaders
    Shader shader_sky_atmosphere = Shader("shaders/sky_atmosphere_1993.vert", "shaders/sky_atmosphere_1993.frag");
    SkyAtmosphere1993Parameters parameters;

private:
    std::string name = "sky_atmosphere";

};
