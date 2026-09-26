#include "components/PBRIBL_Spheres.h"
#include "utils/RenderProfiler.h"
#include "core/Window.h"
#include "scene/GameObject.h"

void PBRIBL_Spheres::Load(Window& window)
{
    PBRIBL_Base::Load(window);  // 执行 glEnable 等

    const int pbr_test_grid = 6;
    const float spacing = 1.4f;
    const glm::vec3 origin = glm::vec3(3.0f, 3.0f, -3.0f);

    for (int row = 0; row < pbr_test_grid; row++)
    {
        for (int col = 0; col < pbr_test_grid; col++)
        {
            std::string name = "sphere_" + std::to_string(row) + "_" + std::to_string(col);
            GameObject& s = scene.AddGameObject(name, &sphereModel, &shader_pbr_ibl_test);
            s.SetPosition(origin + glm::vec3(0.0f, -row * spacing, col * spacing));
            s.SetScale(1.2f);
            s.pbr_sphere = PBRTestComponent{};
            s.pbr_sphere->albedo = glm::vec3(1.0f);
            s.pbr_sphere->roughness = glm::clamp((float)row / (pbr_test_grid - 1), 0.05f, 1.0f);
            s.pbr_sphere->metallic = (float)col / (pbr_test_grid - 1);
        }
    }

    scene.SetSelected(scene.GetGameObjects()[0]->GetID());
}

void PBRIBL_Spheres::Render(Camera& camera, RenderProfiler& profiler)
{
    // shader_pbr_ibl_test
    ibl.Bind(shader_pbr_ibl_test);
    shader_pbr_ibl_test.setMat4("view", camera.GetView());
    shader_pbr_ibl_test.setMat4("projection", camera.GetProjection());
    shader_pbr_ibl_test.setVec3("light.ambient", ambient);
    shader_pbr_ibl_test.setVec3("light.intensity", glm::vec3(0.0f, 0.0f, 0.0f));
    shader_pbr_ibl_test.setVec3("viewPos", camera.GetPosition());
    shader_pbr_ibl_test.setVec3("lightPos", glm::vec3(0.0f, 0.0f, 0.0f));
    shader_pbr_ibl_test.setVec3("baseColor", sphere_albedo);

    {
        A3_PROFILE_PASS(profiler, "Opaque Scene");
        scene.Render(camera, profiler);
    }

    PBRIBL_Base::Render(camera, profiler);

}

void PBRIBL_Spheres::Unload()
{
    scene.Clear();
}


void PBRIBL_Spheres::RenderEditor(Editor& editor)
{
    PBRIBL_Base::RenderEditor(editor);
}
