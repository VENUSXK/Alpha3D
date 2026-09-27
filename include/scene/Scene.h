#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "renderer/IBL.h"

class Camera;
class Editor;
class GameObject;
class Model;
class RenderProfiler;
class SkyAtmosphere;
class Shader;
class VolumetricCloud;
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
    std::vector<std::unique_ptr<GameObject>> game_objects;
    std::unique_ptr<SkyAtmosphere> atmosphere;
    std::unique_ptr<VolumetricCloud> volumetric_cloud;

    uint32_t selected_id = 0;
    uint32_t next_id = 1;
    IBL ibl;

    std::string name = "landscape";
};
