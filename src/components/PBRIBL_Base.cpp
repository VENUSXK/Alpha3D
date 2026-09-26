#include "components/PBRIBL_Base.h"
#include "utils/RenderProfiler.h"
#include "core/Editor.h"
#include "core/Camera.h"

void PBRIBL_Base::Load(Window& window)
{
    BaseScene::Load(window); // glEnable
    ibl.Scan("assets/hdri");
    {
        ibl.Load(ibl.GetSelectedPath(), cubeModel, to_cubemap_shader,
                 irradiance_shader, prefilter_shader, brdf_integrate_shader);
    }
}

void PBRIBL_Base::Render(Camera& camera, RenderProfiler& profiler)
{
    A3_PROFILE_PASS(profiler, "Skybox");
    // skybox
    glDepthFunc(GL_LEQUAL);
    skybox_shader.use();
    skybox_shader.setMat4("projection", camera.GetProjection());
    skybox_shader.setMat4("view", camera.GetView());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ibl.envCubemap);
    skybox_shader.setInt("environmentMap", 0);
    cubeModel.Draw(skybox_shader);
    glDepthFunc(GL_LESS);
}

void PBRIBL_Base::Unload()
{
    scene.Clear();
}


void PBRIBL_Base::RenderEditor(Editor& editor)
{

    editor.BeginEnvironment(ibl);
    if (ibl.HasChanged()) {
        ibl.Load(
            ibl.GetSelectedPath(), cubeModel,
            to_cubemap_shader, irradiance_shader,
            prefilter_shader, brdf_integrate_shader
        );
        ibl.ClearChanged();
    }

    BaseScene::RenderEditor(editor);

}
