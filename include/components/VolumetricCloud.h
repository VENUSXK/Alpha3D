#pragma once

#include <glm/glm.hpp>

#include "renderer/Model.h"
#include "renderer/Shader.h"

class Camera;
class Editor;
class GameObject;
class RenderProfiler;
class Scene;
class SkyAtmosphere;
class SunLight;
class Window;

struct VolumetricCloudParameters {
    float densityScale = 0.45f;
    float extinction = 0.1f;
    float cloudOpacity = 1.0f;

    bool useLowFreqNoise = true;
    bool useHighFreqNoise = true;
    bool useHeightProfile = true;
    bool useBaseReduction = true;
    glm::vec4 stratusProfile = glm::vec4(0.0f, 0.1f, 0.2f, 0.3f);
    glm::vec4 stratocumulusProfile = glm::vec4(0.0f, 0.2f, 0.3f, 0.5f);
    glm::vec4 cumulusProfile = glm::vec4(0.0f, 0.1f, 0.75f, 1.0f);
    float erosionStrength = 0.5f;

    float lowFreqNoiseScale = 680.0f;
    float highFreqNoiseScale = 680.0f;

    float cloudMapScale = 10000.0f;

    glm::vec3 windDirection = glm::vec3(0.0f, 0.0f, 1.0f);
    float windSpeed = 20.0f;
    float noiseSpeed = 40.0f;
    float cloudTopOffset = 0.0f;
    float anvilBias = 0.0f;
    float cloudCoverageBlend = 0.5f;
    bool animateCoverageBlend = false;
    float coverageBlendAmplitude = 0.2f;
    float coverageBlendSpeed = 1.0f;

    float ambientLightIntensity = 0.29f;
    float sunLightMultiplier = 2.98f;

    int primarySteps = 48;
    int lightSteps = 32;
    float rayJitterStrength = 1.0f;
    float transmittanceCutoff = 0.01f;

    float phaseG = 0.7f;
    float silverIntensity = 2.0f;
    float silverSpread = 0.7f;
    float heightExponentTop = 1.5f;
    float lightingDensityLodScale = 1.0f;

    glm::vec3 cloudMapVolumeScale = glm::vec3(40000.0f, 800.0f, 40000.0f);
    glm::vec3 cloudMapVolumeTranslation = glm::vec3(0.0f, 2000.0f, 0.0f);
};

class VolumetricCloud {
public:
    void Load(Window& window, Scene& scene);
    void Render(Camera& camera, RenderProfiler& profiler, const SunLight& sunLight, const SkyAtmosphere& atmosphere, bool useAerialPerspective, bool outputLinear);
    void Unload();
    void RenderEditor(Editor& editor);
    void MarkParametersDirty() {}

    void updateVolumeTransform(const glm::vec3& scale, const glm::vec3& translation);

    VolumetricCloudParameters parameters;

private:
    Shader shader_volumetric_cloud = Shader(
        "shaders/volumetric_cloud/cube.vert",
        "shaders/volumetric_cloud/volumetric_cloud.frag"
    );
    Model cubeModel = Model::Cube();
    GameObject* cloudVolume = nullptr;
    bool loaded = false;

    unsigned int lowFreqNoiseTex = 0;
    unsigned int highFreqNoiseTex = 0;
    unsigned int cloudMapTex = 0;
    unsigned int heightTex = 0;
};
