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

#include "renderer/IBL.h"

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

    terrain = std::make_unique<Model>(Model::Terrain("assets/textures/height-map.png", 10000.0f, 10000.0f, 3000.0f, {}));

    auto& object = AddGameObject("Terrain", terrain.get(), &shader);
    object.SetPosition(glm::vec3(0.0f, -100.0f, 0.0f));
    object.SetScale(1.0f);
}

void Scene::Render(Camera& camera, RenderProfiler& profiler)
{
    RenderSkyLight(camera, profiler, false);

    if (ibl.envCubemap == 0){
        ibl.Render(*this, camera, profiler);
    }

    // terrain testing

    ibl.Bind(shader);

    shader.setMat4("view", camera.GetView());
    shader.setMat4("projection", camera.GetProjection());
    shader.setVec3("viewPos", camera.GetPosition());

    shader.setVec3("baseColor", glm::vec3(0.35f, 0.45f, 0.2f));
    shader.setFloat("roughness", 0.8f);
    shader.setFloat("metallic", 0.0f);
    shader.setMat3("environmentRotation", glm::mat3(1.0f));

    shader.setVec3("lightPos", glm::vec3(0.0f, 1000.0f, 0.0f));
    shader.setVec3("light.intensity", glm::vec3(1000000.0f));

    for (const auto& object : game_objects) {
        object->Draw();
    }
}


void Scene::RenderSkyLight(Camera& camera, RenderProfiler& profiler, bool outputLinear) {
    A3_PROFILE_PASS(profiler, "SkyLight");

    const GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
    GLboolean depthWriteEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);
    atmosphere->Render(camera, profiler, outputLinear);

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

void Scene::RenderEditor(Editor& editor, Camera& camera) {
    editor.RenderMaterialPreview(this->GetIBL(), camera);
    editor.BeginSkyAtmosphere(*atmosphere);
    editor.BeginVolumetricCloud(*volumetric_cloud);
    editor.BeginIBL();
    editor.BeginHierarchy(*this);
    if (GameObject* selected = GetSelected()) editor.BeginDetails(*selected);
}

SkyAtmosphere& Scene::GetAtmosphere() { return *atmosphere; }

const SkyAtmosphere& Scene::GetAtmosphere() const { return *atmosphere; }

VolumetricCloud& Scene::GetVolumetricCloud() { return *volumetric_cloud; }

const VolumetricCloud& Scene::GetVolumetricCloud() const { return *volumetric_cloud; }
