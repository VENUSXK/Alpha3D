#include "components/PBRIBL_SingleTex.h"
#include "utils/RenderProfiler.h"
#include "core/Window.h"
#include "scene/GameObject.h"
#include "renderer/Model.h"

PBRIBL_SingleTex::PBRIBL_SingleTex(std::string path, std::string inst_name)
    : model_path(path), model((project_path + path).c_str()){
    name = inst_name;
}

void PBRIBL_SingleTex::Load(Window& window)
{
    PBRIBL_Base::Load(window); 
    // Scene
    game_object = &scene.AddGameObject("industrial_microscope", &model, &shader_pbr_ibl_object);
    game_object->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    game_object->SetScale(10.0f);
    scene.SetSelected(game_object->GetID());
}

void PBRIBL_SingleTex::Render(Camera& camera, RenderProfiler& profiler)
{
    // shader_pbr
    ibl.Bind(shader_pbr_ibl_object);
    shader_pbr_ibl_object.setBool("isLight", false);
    shader_pbr_ibl_object.setMat4("view", camera.GetView());
    shader_pbr_ibl_object.setMat4("projection", camera.GetProjection());
    shader_pbr_ibl_object.setVec3("light.intensity", glm::vec3(0, 0, 0));
    shader_pbr_ibl_object.setVec3("F0", glm::vec3(0.3f));
    shader_pbr_ibl_object.setVec3("viewPos", camera.GetPosition());
    shader_pbr_ibl_object.setVec3("lightPos", glm::vec3(0, 0, 0));


    {
        A3_PROFILE_PASS(profiler, "Opaque Scene");
        scene.Render(camera, profiler);
    }

    PBRIBL_Base::Render(camera, profiler);

}

void PBRIBL_SingleTex::Unload()
{
    scene.Clear();
}


void PBRIBL_SingleTex::RenderEditor(Editor& editor)
{
    PBRIBL_Base::RenderEditor(editor);
}
