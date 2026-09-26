#pragma once
#include "renderer/Shader.h"
#include "renderer/IBL.h"
#include "components/BaseScene.h"

class Window;

class PBRIBL_Base : public BaseScene
{
public:
    void Load(Window& window);
    void Render(Camera& camera, RenderProfiler& profiler);
    void Unload();
    void RenderEditor(Editor& editor);

    GameObject* GetMainGameObject() { return nullptr; }

    // Shaders
    Shader shader_pbr_ibl_object = Shader("shaders/pbr/pbr_ibl.vert", "shaders/pbr/pbr_ibl.frag");
    Shader shader_pbr_ibl_light = Shader("shaders/pbr/pbr_ibl.vert", "shaders/pbr/pbr_ibl.frag");
    Shader shader_pbr_ibl_test = Shader("shaders/pbr/pbr_ibl_test.vert", "shaders/pbr/pbr_ibl_test.frag");
    Shader to_cubemap_shader = Shader("shaders/pbr/equirect_to_cubemap.vert", "shaders/pbr/equirect_to_cubemap.frag");
    Shader skybox_shader = Shader("shaders/pbr/skybox.vert", "shaders/pbr/skybox.frag");
    Shader irradiance_shader = Shader("shaders/pbr/irradiance_convolution.vert", "shaders/pbr/irradiance_convolution.frag");
    Shader prefilter_shader = Shader("shaders/pbr/prefilter_convolution.vert", "shaders/pbr/prefilter_convolution.frag");
    Shader brdf_integrate_shader = Shader("shaders/pbr/brdf_integrate.vert", "shaders/pbr/brdf_integrate.frag");

    IBL ibl;

};
