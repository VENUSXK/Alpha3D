#include "components/PBRIBL_Custom.h"
#include "core/Window.h"
#include "scene/GameObject.h"
#include "utils/RenderProfiler.h"

void PBRIBL_Custom::Load(Window& window)
{

    PBRIBL_Base::Load(window);  // 执行 glEnable 等

    // Scene
    sphere = &scene.AddGameObject("PBR_sphere", &sphereModel, &shader_pbr_ibl_test);
    sphere->SetPosition(glm::vec3(2.0f, 1.5f, 0.0f));
    sphere->SetScale(2.0f);
    sphere->pbr_sphere = PBRTestComponent{};

    ginger = &scene.AddGameObject("ginger", &model_ginger, &shader_pbr_ibl_object);
    ginger->SetScale(5.0f);
    ginger->SetPosition(glm::vec3(0.560f, -0.440f, 1.490f));
    ginger->SetRotation(glm::vec3(0.0f, 22.5f, 0.0f));

    bulb = &scene.AddGameObject("bulb", &model_bulb, &shader_pbr_ibl_light);
    bulb->SetPosition(glm::vec3(0.0f));
    bulb->SetScale(10.0f);
    bulb->light = LightComponent{};

    scene.SetSelected(bulb->GetID());
}

void PBRIBL_Custom::Render(Camera& camera, RenderProfiler& profiler)
{
    glm::vec4 worldLightPos = bulb->GetTransform().matrix * glm::vec4(localLightPos, 1.0f);
    glm::vec3 lightPos = glm::vec3(worldLightPos);

    // shader_pbr
    ibl.Bind(shader_pbr_ibl_object);
    shader_pbr_ibl_object.setBool("isLight", false);
    shader_pbr_ibl_object.setMat4("view", camera.GetView());
    shader_pbr_ibl_object.setMat4("projection", camera.GetProjection());
    shader_pbr_ibl_object.setVec3("light.ambient", ambient);
    shader_pbr_ibl_object.setVec3("light.intensity", glm::vec3(bulb->light->intensity * bulb->light->color));
    shader_pbr_ibl_object.setVec3("F0", glm::vec3(0.3f));
    shader_pbr_ibl_object.setVec3("viewPos", camera.GetPosition());
    shader_pbr_ibl_object.setVec3("lightPos", lightPos);

    // shader_light
    ibl.Bind(shader_pbr_ibl_light);
    shader_pbr_ibl_light.setBool("isLight", true);
    shader_pbr_ibl_light.setBool("lightOn", true);
    shader_pbr_ibl_light.setVec3("emissiveIntensity", bulb->light->intensity * bulb->light->color / 2.0f);
    shader_pbr_ibl_light.setMat4("view", camera.GetView());
    shader_pbr_ibl_light.setMat4("projection", camera.GetProjection());
    shader_pbr_ibl_light.setVec3("light.ambient", ambient);
    shader_pbr_ibl_light.setVec3("light.intensity", glm::vec3(bulb->light->intensity * bulb->light->color));
    shader_pbr_ibl_light.setVec3("F0", glm::vec3(0.3f));
    shader_pbr_ibl_light.setVec3("viewPos", camera.GetPosition());
    shader_pbr_ibl_light.setVec3("lightPos", lightPos);

    // shader_pbr_ibl_test
    ibl.Bind(shader_pbr_ibl_test);
    shader_pbr_ibl_test.setMat4("view", camera.GetView());
    shader_pbr_ibl_test.setMat4("projection", camera.GetProjection());
    shader_pbr_ibl_test.setVec3("light.intensity", glm::vec3(bulb->light->intensity * bulb->light->color));
    shader_pbr_ibl_test.setVec3("viewPos", camera.GetPosition());
    shader_pbr_ibl_test.setVec3("lightPos", lightPos);

    {
        A3_PROFILE_PASS(profiler, "Opaque Scene");
        scene.Render(camera, profiler);
    }

    PBRIBL_Base::Render(camera, profiler);

}

void PBRIBL_Custom::Unload()
{
    scene.Clear();
}


void PBRIBL_Custom::RenderEditor(Editor& editor)
{
    PBRIBL_Base::RenderEditor(editor);
}
