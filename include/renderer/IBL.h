#pragma once

#include "renderer/Model.h"
#include "renderer/Shader.h"

class Scene;
class Camera;
class RenderProfiler;

class IBL {
public:
    unsigned int envCubemap = 0;
    unsigned int irradianceMap = 0;
    unsigned int prefilterMap = 0;
    unsigned int brdfLUTTexture = 0;

    void Render(Scene& scene, Camera& camera, RenderProfiler& profiler);
    void Bind(Shader& shader) const;

    IBL() = default;
    ~IBL();

    IBL(const IBL&) = delete;
    IBL& operator=(const IBL&) = delete;
private:

 
    Model cubeModel = Model::Cube();

    Shader irradiance_shader{ "shaders/pbr/irradiance_convolution.vert", "shaders/pbr/irradiance_convolution.frag" };
    Shader prefilter_shader{ "shaders/pbr/prefilter_convolution.vert", "shaders/pbr/prefilter_convolution.frag" };
    Shader brdf_integrate_shader{ "shaders/pbr/brdf_integrate.vert", "shaders/pbr/brdf_integrate.frag" };

    unsigned int captureFBO = 0;
    unsigned int captureRBO = 0;
};
