#include "scene/Scene.h"

#include "components/SkyAtmosphere.h"
#include "components/VolumetricCloud.h"
#include "core/Editor.h"
#include "core/opengl.h"
#include "core/Window.h"
#include "renderer/Camera.h"
#include "renderer/Model.h"
#include "renderer/Shader.h"
#include "scene/GameObject.h"
#include "utils/RenderProfiler.h"

Scene::Scene() = default;

Scene::~Scene() = default;

void Scene::Clear() {
    game_objects.clear();
    selected_id = 0;
    next_id = 1;
}

void Scene::Load(Window& window) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    if (!atmosphere) atmosphere = std::make_unique<SkyAtmosphere>();
    if (!volumetric_cloud) volumetric_cloud = std::make_unique<VolumetricCloud>();

    atmosphere->Load(window);
    volumetric_cloud->Load(window, *this);
}

void Scene::Render(Camera& camera, RenderProfiler& profiler) {
    A3_PROFILE_PASS(profiler, "Scene");

    const GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthWriteEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    atmosphere->Render(camera, profiler);

    glDepthMask(depthWriteEnabled);
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    volumetric_cloud->Render(camera, profiler);

    if (blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
}

void Scene::Unload() {
    if (volumetric_cloud) volumetric_cloud->Unload();
    if (atmosphere) atmosphere->Unload();
    Clear();
}

GameObject& Scene::AddGameObject(std::string name, Model* model, Shader* shader) {
    game_objects.push_back(std::make_unique<GameObject>(next_id++, std::move(name), model, shader));
    return *game_objects.back();
}

GameObject* Scene::GetSelected() {
    for (auto& gameObject : game_objects) if (gameObject->GetID() == selected_id) return gameObject.get();
    return nullptr;
}

void Scene::RenderEditor(Editor& editor) {
    editor.BeginSkyAtmosphere(*atmosphere);
    editor.BeginVolumetricCloud(*volumetric_cloud);
    editor.BeginHierarchy(*this);
    if (GameObject* selected = GetSelected()) editor.BeginDetails(*selected);
}

SkyAtmosphere& Scene::GetAtmosphere() { return *atmosphere; }

const SkyAtmosphere& Scene::GetAtmosphere() const { return *atmosphere; }

VolumetricCloud& Scene::GetVolumetricCloud() { return *volumetric_cloud; }

const VolumetricCloud& Scene::GetVolumetricCloud() const { return *volumetric_cloud; }
