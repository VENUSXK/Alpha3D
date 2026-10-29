#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "renderer/IBL.h"
#include "components/SunLight.h"

class Camera;
class Editor;
class GameObject;
class Model;
class RenderProfiler;
class SkyAtmosphere;
class Shader;
class VolumetricCloud;
class Terrain;

class Window;

class Scene {
public:
    Scene();
    ~Scene();

    void Load(Window& window);
    void RenderSkyLight(Camera& camera, RenderProfiler& profiler, bool outputLinear);
    void Render(Camera& camera, RenderProfiler& profiler);
    void Unload();
    void RenderEditor(Editor& editor, Camera& camera);


    const IBL& GetIBL() const { return ibl; }
    IBL& GetIBL() { return ibl; }
    const SunLight& GetSunLight() const { return sunLight; }
    SunLight& GetSunLight() { return sunLight; }
    const std::string& GetName() const { return name; }
    SkyAtmosphere& GetAtmosphere();
    const SkyAtmosphere& GetAtmosphere() const;
    VolumetricCloud& GetVolumetricCloud();
    const VolumetricCloud& GetVolumetricCloud() const;

    GameObject& AddGameObject(std::string name, Model* model, Shader* shader);
    void SetSelected(uint32_t id) { selected_id = id; }
    GameObject* GetSelected();
    void ClearSelection() { selected_id = 0; }
    const std::vector<std::unique_ptr<GameObject>>& GetGameObjects() const { return game_objects; }
    void Clear();


private:
    void RenderSky(Camera& camera, RenderProfiler& profiler, bool outputLinear, bool useSceneShadow);
    void RenderCloud(Camera& camera, RenderProfiler& profiler, bool useAerialPerspective, bool outputLinear);

    Shader shadowShader{ "shaders/shadow/shadow.vert", "shaders/shadow/shadow.frag" };
    Shader testShader{ "shaders/pbr/pbr_ibl_test.vert", "shaders/pbr/pbr_ibl_test.frag" };


    std::unique_ptr<Model> testModel;
    GameObject* testObject = nullptr;

    unsigned int shadowTexture = 0;
    unsigned int shadowFramebuffer = 0;
    int shadowSize = 8192;

    std::vector<std::unique_ptr<GameObject>> game_objects;
    std::unique_ptr<SkyAtmosphere> atmosphere;
    std::unique_ptr<VolumetricCloud> volumetric_cloud;
    std::unique_ptr<Terrain> terrain;
    SunLight sunLight;

    uint32_t selected_id = 0;
    uint32_t next_id = 1;
    IBL ibl;

    std::string name = "landscape";
};
