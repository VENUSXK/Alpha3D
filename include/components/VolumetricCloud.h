#pragma once

#include <glm/glm.hpp>

#include "renderer/Model.h"
#include "renderer/Shader.h"

class Camera;
class Editor;
class GameObject;
class RenderProfiler;
class Scene;
class Window;

struct VolumetricCloudParameters {
    float densityScale = 0.8f;
    float extinction = 0.6f;

    bool useLowFreqNoise = true;
    bool useHighFreqNoise = true;
    float erosionStrength = 0.5f;

    float lowFreqNoiseScale = 68.0f;
    float highFreqNoiseScale = 68.0f;

    float cloudMapScale = 1000.0f;

    glm::vec3 windDirection = glm::vec3(1.0f, 0.0f, 0.0f);
    float cloudSpeed = 2.0f;
    float cloudTopOffset = 0.0f;
    float anvilBias = 0.0f;
    float cloudCoverageBlend = 1.0f;

    glm::vec3 lightDirection = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 ambientLight = glm::vec3(0.65f);
    float lightIntensity = 7.0f;

    int primarySteps = 64;
    int lightSteps = 8;
    float rayJitterStrength = 1.0f;
    float transmittanceCutoff = 0.01f;

    float phaseG = 0.5f;

    glm::vec3 cloudMapVolumeScale = glm::vec3(20000.0f, 60.0f, 20000.0f);
    glm::vec3 cloudMapVolumeTranslation = glm::vec3(0.0f, 260.0f, 0.0f);
};

class VolumetricCloud {
public:
    void Load(Window& window, Scene& scene);
    void Render(Camera& camera, RenderProfiler& profiler);
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
