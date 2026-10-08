#pragma once
#include <functional>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "renderer/LUT.h"
#include "renderer/Shader.h"
class Camera;
class Editor;
class RenderProfiler;
class Window;

struct SkyAtmosphereParams {
    float sunLongitude = -90.0f;
    float sunLatitude = 33.4f;
    float lightIntensity = 40.0f;
    glm::vec3 lightDirection = glm::normalize(glm::vec3(0.0f, 0.55f, -0.84f));
    float cameraHeight = 100.0f;

    // test
    bool useRayleigh = true;
    bool useMie = true;
    bool useAbsorption = true;

    // radius
    float planetRadius = 6371000.0f;
    float atmosphereRadius = 6471000.0f;

    // scattering params
    glm::vec3 rayleighBeta = glm::vec3(5.5e-6f, 13.0e-6f, 22.4e-6f);
    glm::vec3 mieBeta = glm::vec3(21.0e-6f);
    glm::vec3 absorptionBeta = glm::vec3(2.04e-5f, 4.97e-5f, 1.95e-6f);

    // scattering
    float rayleighHeight = 8000.0f;
    float mieHeight = 1200.0f;
    float absorptionHeight = 30000.0f;
    float absorptionFalloff = 4000.0f;
    float mieG = 0.7f;

    // post-effects
    float exposure = 1.0f;
    float gamma = 2.2f;

    // ray-marching
    int primarySteps = 128;
    int lightSteps = 8;

    // lut params
    bool useTransmittanceLUT = true;
    bool useSkyViewLUT = true;
    int transmittanceLUTSteps = 64;

    // aerial perspective
    int aerialPerspectiveSize = 32;
    float aerialPerspectiveMaxDistance = 40000.0f;

    int aerialPerspectiveSteps = 32;
};

class SkyAtmosphere {
public:
    void Load(Window& window);
    void Render(Camera& camera, RenderProfiler& profiler, bool outputLinear);
    void Unload();
    void RenderEditor(Editor& editor);
    void MarkParametersDirty() { parametersDirty = true; }

    Shader shader_sky_atmosphere = Shader("shaders/sky_atmosphere/full_screen.vert", "shaders/sky_atmosphere/sky_atmosphere.frag");
    SkyAtmosphereParams parameters;

    void RenderTransmittanceLUT();
    void RenderSkyViewLUT();
    bool SaveTransmittanceLUT(const std::string& outputPath) const;
    bool SaveSkyViewLUT(const std::string& outputPath) const;

    void RenderAerialPerspectiveVolume(Camera& camera);
    void BindAerialPerspectiveVolume(unsigned int slot) const;

    unsigned int shadowTexture = 0;
    glm::mat4 shadowViewProjection = glm::mat4(1.0f);

private:
    LUT* GetLUT(const std::string& name);
    const LUT* GetLUT(const std::string& name) const;
    bool RenderLUT(LUT& lut, Shader& shader, const std::function<void(Shader&)>& configure);
    void RenderFullscreenTriangle();

    std::vector<LUT> luts;
    unsigned int lut_framebuffer = 0;
    bool parametersDirty = true;

    void AllocateAerialPerspectiveVolume();

    unsigned int aerialPerspectiveTexture = 0;
    unsigned int aerialPerspectiveFramebuffer = 0;
    int allocatedAerialPerspectiveSize = 0;
};
