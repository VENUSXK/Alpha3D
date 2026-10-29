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
class SunLight;

class Terrain {
public:
    void Load(Scene& scene);
    void Render(Camera& camera, const IBL& lighting, const SkyAtmosphere& atmosphere, const SunLight& sunLight, RenderProfiler& profiler);
    void Unload();
    GameObject* GetGameObject() const { return terrainObject; }
    Shader terrainShader = Shader("shaders/terrain/terrain.vert", "shaders/terrain/terrain.frag");

private:
    bool useAerialPerspective = true;
    void BindTextures() const;
    std::unique_ptr<Model> terrainModel;
    GameObject* terrainObject = nullptr;
    unsigned int terrainColorTexture = 0;
};
