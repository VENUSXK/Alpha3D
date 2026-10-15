#include "scene/Scene.h"
#include <limits>

#include "components/SkyAtmosphere.h"
#include "components/VolumetricCloud.h"
#include "components/Terrain.h"

#include "core/Editor.h"
#include "core/opengl.h"
#include "core/Window.h"
#include "renderer/Camera.h"
#include "renderer/Model.h"
#include "renderer/Shader.h"
#include "scene/GameObject.h"
#include "utils/RenderProfiler.h"

#include "renderer/IBL.h"
#include "scene/GameObject.h"

Scene::Scene() = default;

Scene::~Scene() = default;

void Scene::Clear() {
    game_objects.clear();
    testObject = nullptr;
    testModel.reset();
    selected_id = 0;
    next_id = 1;
}

void Scene::Load(Window& window) {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    if (!atmosphere) atmosphere = std::make_unique<SkyAtmosphere>();
    if (!volumetric_cloud) volumetric_cloud = std::make_unique<VolumetricCloud>();
    if (!terrain) terrain = std::make_unique<Terrain>();

    atmosphere->Load(window);
    volumetric_cloud->Load(window, *this);
    terrain->Load(*this);

    // test object
    testModel = std::make_unique<Model>("assets/models/stanford_dragon/scene.gltf");
    testObject = &(this->AddGameObject("stanford_dragon", testModel.get(), &testShader));
    testObject->pbr_custom_component = PBRCustomComponent{};
    testObject->pbr_custom_component->albedo = glm::vec3(1.0f, 1.0f, 1.0f);
    testObject->pbr_custom_component->metallic = 1.0f;

    testObject->SetPosition(glm::vec3(0.0f, 452.73f, 0.0f));
    testObject->SetScale(500.0f);

    // shadow texture generation
    glGenTextures(1, &shadowTexture);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, shadowSize, shadowSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glBindTexture(GL_TEXTURE_2D, 0);

    // shadow frame buffer generation
    glGenFramebuffers(1, &shadowFramebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Scene::Render(Camera& camera, RenderProfiler& profiler) {

    std::vector<glm::vec3> shadowCorners;
    shadowCorners.reserve(16);
    for (GameObject* object : { terrain->GetGameObject(), testObject }) {
        if (!object || !object->GetModel()) continue;
        const auto& corners = object->GetCorners();
        shadowCorners.insert(shadowCorners.end(), corners.begin(), corners.end());
    }
    if (shadowCorners.empty()) return;

    glm::vec3 shadowCenter(0.0f);
    for (const glm::vec3& corner : shadowCorners) shadowCenter += corner;
    shadowCenter /= float(shadowCorners.size());
    float shadowRadius = 1.0f;
    for (const glm::vec3& corner : shadowCorners) shadowRadius = glm::max(shadowRadius, glm::length(corner - shadowCenter));
    float shadowMargin = glm::max(shadowRadius * 0.01f, 1.0f);
    glm::vec3 sunDirection = glm::normalize(atmosphere->parameters.lightDirection);
    glm::vec3 shadowUp = glm::abs(sunDirection.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 shadowView = glm::lookAt(shadowCenter + sunDirection * (shadowRadius + shadowMargin + 1.0f), shadowCenter, shadowUp);
    glm::vec3 minPosition(std::numeric_limits<float>::max());
    glm::vec3 maxPosition(-std::numeric_limits<float>::max());
    
    for (const glm::vec3& corner : shadowCorners) {
        glm::vec3 lightPosition = glm::vec3(shadowView * glm::vec4(corner, 1.0f));
        minPosition = glm::min(minPosition, lightPosition);
        maxPosition = glm::max(maxPosition, lightPosition);
    }
    
    float nearPlane = glm::max(0.1f, -maxPosition.z - shadowMargin);
    float farPlane = -minPosition.z + shadowMargin;
    
    glm::mat4 shadowProjection = glm::ortho(minPosition.x - shadowMargin, maxPosition.x + shadowMargin, minPosition.y - shadowMargin, maxPosition.y + shadowMargin, nearPlane, farPlane);
    glm::mat4 shadowViewProjection = shadowProjection * shadowView;

    GLint previousFramebuffer = 0, previousViewport[4] = {}, previousDepthFunction = 0;
    GLboolean previousDepthWrite = GL_TRUE;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunction);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthWrite);

    GLboolean previousDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean previousCull = glIsEnabled(GL_CULL_FACE), previousScissor = glIsEnabled(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);

    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadowFramebuffer);
    glViewport(0, 0, shadowSize, shadowSize);
    glClear(GL_DEPTH_BUFFER_BIT);
    shadowShader.use();
    shadowShader.setMat4("shadowViewProjection", shadowViewProjection);

    for (const auto& object : game_objects) {
        if (object->GetName() == "cloudVolume") continue;
        object->Draw(shadowShader);
    }

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousFramebuffer);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    glDepthFunc(previousDepthFunction);
    glDepthMask(previousDepthWrite);
    if (previousDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);

    if (previousCull) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (previousScissor) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    atmosphere->shadowTexture = shadowTexture;
    atmosphere->shadowViewProjection = shadowViewProjection;


    RenderSkyLight(camera, profiler, false);
    if (ibl.IsDirty()) ibl.Render(*this, camera, profiler);

    atmosphere->RenderAerialPerspectiveVolume(camera);
    terrain->Render(camera, ibl, *atmosphere, profiler);

    testShader.use();
    testShader.setMat4("view", camera.GetView());
    testShader.setMat4("projection", camera.GetProjection());
    testShader.setVec3("viewPos", camera.GetPosition());

    testShader.setBool("useDirectionalLight", true);
    testShader.setVec3("sunDirection", atmosphere->getLightDirection());
    testShader.setVec3("light.intensity", glm::vec3(atmosphere->getLightIntensity()));

    testShader.setMat3("environmentRotation", glm::mat3(1.0f));
    ibl.Bind(testShader);
    testShader.setBool("useAerialPerspective", atmosphere->parameters.useAerialPerspective);
    testShader.setVec3("aerialPerspectiveViewportSize", glm::vec3(float(previousViewport[2]), float(previousViewport[3]), 0.0f));
    testShader.setFloat("aerialPerspectiveMaxDistance", atmosphere->parameters.aerialPerspectiveMaxDistance);
    testShader.setInt("aerialPerspectiveSliceCount", glm::max(atmosphere->parameters.aerialPerspectiveSize, 1));
    testShader.setInt("aerialPerspectiveVolume", 12);
    atmosphere->BindAerialPerspectiveVolume(12);

    testShader.setBool("useSurfaceShadow", true);
    testShader.setMat4("shadowViewProjection", shadowViewProjection);
    testShader.setInt("surfaceShadowMap", 13);
    glActiveTexture(GL_TEXTURE13);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);

    for (const auto& object : game_objects) {
        if (object.get() == terrain->GetGameObject()) continue;
        if (object->GetName() == "cloudVolume") continue;
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
    glDeleteTextures(1, &shadowTexture);
    glDeleteFramebuffers(1, &shadowFramebuffer);
    shadowTexture = 0;
    shadowFramebuffer = 0;

    if (volumetric_cloud) volumetric_cloud->Unload();
    if (atmosphere) atmosphere->Unload();
    Clear();
    if (terrain) terrain->Unload();
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
    editor.BeginIBL(ibl);
    editor.BeginHierarchy(*this);
    if (terrain) editor.BeginTerrain(*terrain);
    if (GameObject* selected = GetSelected()) editor.BeginDetails(*selected);
}

SkyAtmosphere& Scene::GetAtmosphere() { return *atmosphere; }

const SkyAtmosphere& Scene::GetAtmosphere() const { return *atmosphere; }

VolumetricCloud& Scene::GetVolumetricCloud() { return *volumetric_cloud; }

const VolumetricCloud& Scene::GetVolumetricCloud() const { return *volumetric_cloud; }
