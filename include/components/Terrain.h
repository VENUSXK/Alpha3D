#pragma once
#include <array>
#include <memory>
#include <glm/glm.hpp>
#include "renderer/Model.h"
#include "renderer/Shader.h"

class Camera;
class GameObject;
class IBL;
class RenderProfiler;
class Scene;
class SkyAtmosphere;

struct TerrainParams {
    glm::vec3 textureSizeMeters = glm::vec3(40.0f, 40.0f, 20.0f);
    glm::vec3 lightPosition = glm::vec3(0.0f, 1000.0f, 0.0f);
    glm::vec3 lightIntensity = glm::vec3(0.0f);
    bool useAerialPerspective = true;

    float rockThreshold = 0.37f;
    float grassThreshold = 0.8f;

    float marbleToDarkThreshold = 0.45f;
    float darkToRockyThreshold = 0.55f;
};

class Terrain {
public:
    void Load(Scene& scene);
    void Render(Camera& camera, const IBL& lighting, const SkyAtmosphere& atmosphere, RenderProfiler& profiler);
    void Unload();
    GameObject* GetGameObject() const { return terrainObject; }

    TerrainParams parameters;
    Shader terrainShader = Shader("shaders/terrain/terrain.vert", "shaders/terrain/terrain.frag");

private:
    void BindTextures() const;
    std::unique_ptr<Model> terrainModel;
    GameObject* terrainObject = nullptr;
    unsigned int terrainColorTexture = 0;
};