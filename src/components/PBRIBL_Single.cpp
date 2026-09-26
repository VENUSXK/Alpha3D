#include "components/PBRIBL_Single.h"
#include "utils/RenderProfiler.h"
#include "core/Window.h"
#include "scene/GameObject.h"
#include "renderer/Model.h"

PBRIBL_Single::PBRIBL_Single(std::string path, std::string inst_name)
    : model_path(path), model((project_path + path).c_str()){
    name = inst_name;
}

void PBRIBL_Single::Load(Window& window)
{
    PBRIBL_Base::Load(window); 
    // Scene
    game_object = &scene.AddGameObject("industrial_microscope", &model, &shader_pbr_ibl_test);
    game_object->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    game_object->SetScale(10.0f);
    game_object->pbr_sphere = PBRTestComponent{};
    scene.SetSelected(game_object->GetID());
}

void PBRIBL_Single::Render(Camera& camera, RenderProfiler& profiler)
{
    // shader_pbr
    ibl.Bind(shader_pbr_ibl_test);
    shader_pbr_ibl_test.setBool("isLight", false);
    shader_pbr_ibl_test.setMat4("view", camera.GetView());
    shader_pbr_ibl_test.setMat4("projection", camera.GetProjection());
    shader_pbr_ibl_test.setVec3("light.intensity", glm::vec3(0, 0, 0));
    shader_pbr_ibl_test.setVec3("F0", glm::vec3(0.3f));
    shader_pbr_ibl_test.setVec3("viewPos", camera.GetPosition());
    shader_pbr_ibl_test.setVec3("lightPos", glm::vec3(0, 0, 0));

    {
        A3_PROFILE_PASS(profiler, "Opaque Scene");
        scene.Render(camera, profiler);
    }

    PBRIBL_Base::Render(camera, profiler);

}

void PBRIBL_Single::Unload()
{
    scene.Clear();
}


void PBRIBL_Single::RenderEditor(Editor& editor)
{
    PBRIBL_Base::RenderEditor(editor);
}
