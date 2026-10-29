#include "components/VolumetricCloud.h"

#include "components/SkyAtmosphere.h"
#include "core/Editor.h"
#include "core/opengl.h"
#include "core/Window.h"
#include "renderer/Camera.h"
#include "components/SunLight.h"
#include "renderer/Texture.h"
#include "scene/GameObject.h"
#include "scene/Scene.h"
#include "utils/RenderProfiler.h"

void VolumetricCloud::updateVolumeTransform(const glm::vec3& scale,
                                            const glm::vec3& translation) {
    cloudVolume->GetTransform().SetScale(scale);
    cloudVolume->GetTransform().SetPosition(translation);
}

void VolumetricCloud::Load(Window&, Scene& scene) {
    cloudVolume = &scene.AddGameObject("cloudVolume", &cubeModel, &shader_volumetric_cloud);
    scene.SetSelected(cloudVolume->GetID());

    cloudVolume->Translate(parameters.cloudMapVolumeTranslation);
    cloudVolume->Scale(parameters.cloudMapVolumeScale);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    loaded = true;
    loaded &= Texture::Load3D(
        lowFreqNoiseTex, 128,
        "assets/textures/nubis2017_lowFrexqTex_128x128/nubis2017.%03d.tga"
    );
    loaded &= Texture::Load3D(
        highFreqNoiseTex, 32,
        "assets/textures/nubis2017_highFrexqTex_32x32/nubis-2017-high-freq.%03d.tga"
    );
    loaded &= Texture::Load2D(heightTex, "assets/textures/cloud_height.tga");
    loaded &= Texture::Load2D(cloudMapTex, "assets/textures/cloud_map.png");
    glBindTexture(GL_TEXTURE_2D, cloudMapTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void VolumetricCloud::Render(Camera& camera, RenderProfiler& profiler, const SunLight& sunLight, const SkyAtmosphere& atmosphere, bool useAerialPerspective, bool outputLinear) {
    if (!loaded || !cloudVolume) return;

    A3_PROFILE_PASS(profiler, "Volumetric Cloud Cube");
    shader_volumetric_cloud.use();

    const glm::mat4 model = cloudVolume->GetTransform().GetModelMatrix();
    const glm::mat4 inverseModel = glm::inverse(model);

    shader_volumetric_cloud.setMat4("model", model);
    shader_volumetric_cloud.setMat4("inverseModel", inverseModel);
    shader_volumetric_cloud.setMat4("view", camera.GetView());
    shader_volumetric_cloud.setMat4("projection", camera.GetProjection());
    shader_volumetric_cloud.setVec3("cameraPos", camera.GetPosition());
    shader_volumetric_cloud.setMat4("invProjection", glm::inverse(camera.GetProjection()));
    shader_volumetric_cloud.setMat4("invView", glm::inverse(camera.GetView()));
    shader_volumetric_cloud.setBool("outputLinear", outputLinear);
    shader_volumetric_cloud.setFloat("exposure", atmosphere.parameters.exposure);
    shader_volumetric_cloud.setFloat("gamma", atmosphere.parameters.gamma);

    const glm::vec3 lightDirection = glm::length(sunLight.direction) > 0.0001f
        ? glm::normalize(sunLight.direction)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const float ambientFadeStart = glm::sin(glm::radians(-10.0f));
    const float ambientFadeEnd = glm::sin(glm::radians(12.0f));
    const float ambientFactor = glm::smoothstep(ambientFadeStart, ambientFadeEnd, lightDirection.y);
    const float ambientLightIntensity = parameters.ambientLightIntensity * ambientFactor;

    shader_volumetric_cloud.setVec3("lightDirection", lightDirection);
    shader_volumetric_cloud.setVec3("lightColor", sunLight.color);
    shader_volumetric_cloud.setVec3("ambientLight", glm::vec3(1.0f));
    shader_volumetric_cloud.setFloat("ambientLightIntensity", ambientLightIntensity);
    shader_volumetric_cloud.setFloat("lightIntensity", sunLight.intensity * parameters.sunLightMultiplier);
    shader_volumetric_cloud.setFloat("extinction", parameters.extinction);
    shader_volumetric_cloud.setFloat("cloudOpacity", parameters.cloudOpacity);

    shader_volumetric_cloud.setFloat("densityScale", parameters.densityScale);
    shader_volumetric_cloud.setFloat("anvilBias", parameters.anvilBias);
    shader_volumetric_cloud.setFloat("cloudCoverageBlend", parameters.cloudCoverageBlend);
    shader_volumetric_cloud.setBool("animateCoverageBlend", parameters.animateCoverageBlend);
    shader_volumetric_cloud.setFloat("coverageBlendAmplitude", parameters.coverageBlendAmplitude);
    shader_volumetric_cloud.setFloat("coverageBlendSpeed", parameters.coverageBlendSpeed);
    shader_volumetric_cloud.setFloat("cloudMapScale", parameters.cloudMapScale);

    shader_volumetric_cloud.setBool("useLowFreqNoise", parameters.useLowFreqNoise);
    shader_volumetric_cloud.setBool("useHighFreqNoise", parameters.useHighFreqNoise);
    shader_volumetric_cloud.setBool("useHeightProfile", parameters.useHeightProfile);
    shader_volumetric_cloud.setBool("useBaseReduction", parameters.useBaseReduction);
    shader_volumetric_cloud.setVec4("stratusProfile", parameters.stratusProfile);
    shader_volumetric_cloud.setVec4("stratocumulusProfile", parameters.stratocumulusProfile);
    shader_volumetric_cloud.setVec4("cumulusProfile", parameters.cumulusProfile);
    shader_volumetric_cloud.setFloat("lowFreqNoiseScale", parameters.lowFreqNoiseScale);
    shader_volumetric_cloud.setFloat("highFreqNoiseScale", parameters.highFreqNoiseScale);
    shader_volumetric_cloud.setFloat("erosionStrength", parameters.erosionStrength);

    shader_volumetric_cloud.setFloat("windSpeed", parameters.windSpeed);
    shader_volumetric_cloud.setFloat("noiseSpeed", parameters.noiseSpeed);
    shader_volumetric_cloud.setVec3("windDirection", parameters.windDirection);
    shader_volumetric_cloud.setFloat("time", static_cast<float>(glfwGetTime()));
    shader_volumetric_cloud.setFloat("cloudTopOffset", parameters.cloudTopOffset);

    shader_volumetric_cloud.setInt("primarySteps", parameters.primarySteps);
    shader_volumetric_cloud.setInt("lightSteps", parameters.lightSteps);
    shader_volumetric_cloud.setFloat("rayJitterStrength", parameters.rayJitterStrength);
    shader_volumetric_cloud.setFloat("transmittanceCutoff", parameters.transmittanceCutoff);
    shader_volumetric_cloud.setFloat("phaseG", parameters.phaseG);
    shader_volumetric_cloud.setFloat("silverIntensity", parameters.silverIntensity);
    shader_volumetric_cloud.setFloat("silverSpread", parameters.silverSpread);
    shader_volumetric_cloud.setFloat("heightExponentTop", parameters.heightExponentTop);
    shader_volumetric_cloud.setFloat("lightingDensityLodScale", parameters.lightingDensityLodScale);

    const bool applyAerialPerspective = useAerialPerspective && atmosphere.parameters.useAerialPerspective;
    shader_volumetric_cloud.setBool("useAerialPerspective", applyAerialPerspective);
    shader_volumetric_cloud.setFloat("aerialPerspectiveMaxDistance", atmosphere.parameters.aerialPerspectiveMaxDistance);
    shader_volumetric_cloud.setFloat("aerialPerspectiveDistanceScale", atmosphere.parameters.cloudAerialPerspectiveDistanceScale);
    shader_volumetric_cloud.setInt("aerialPerspectiveSliceCount", glm::max(atmosphere.parameters.aerialPerspectiveSize, 1));
    shader_volumetric_cloud.setInt("aerialPerspectiveVolume", 4);

    shader_volumetric_cloud.setInt("lowFreqNoiseTex", 0);
    shader_volumetric_cloud.setInt("highFreqNoiseTex", 1);
    shader_volumetric_cloud.setInt("cloudMapTex", 2);
    shader_volumetric_cloud.setInt("heightTex", 3);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, lowFreqNoiseTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, highFreqNoiseTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, cloudMapTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, heightTex);
    if (applyAerialPerspective) atmosphere.BindAerialPerspectiveVolume(4);
    glActiveTexture(GL_TEXTURE0);

    const GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
    const GLboolean depthClampEnabled = glIsEnabled(GL_DEPTH_CLAMP);
    GLboolean depthWriteEnabled = GL_TRUE;
    GLint previousCullMode = 0;
    GLint previousDepthFunction = 0;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);
    glGetIntegerv(GL_CULL_FACE_MODE, &previousCullMode);
    glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunction);

    const glm::vec3 localCameraPos = glm::vec3(
        inverseModel * glm::vec4(camera.GetPosition(), 1.0f)
    );
    constexpr float epsilon = 0.0001f;
    const bool cameraInsideVolume =
        localCameraPos.x >= -0.5f - epsilon && localCameraPos.x <= 0.5f + epsilon &&
        localCameraPos.y >= -0.5f - epsilon && localCameraPos.y <= 0.5f + epsilon &&
        localCameraPos.z >= -0.5f - epsilon && localCameraPos.z <= 0.5f + epsilon;

    glEnable(GL_DEPTH_CLAMP);
    glEnable(GL_CULL_FACE);
    glCullFace(cameraInsideVolume ? GL_FRONT : GL_BACK);
    glDepthMask(GL_FALSE);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    cloudVolume->Draw();

    glDepthMask(depthWriteEnabled);
    glDepthFunc(previousDepthFunction);
    glCullFace(previousCullMode);
    if (cullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (depthClampEnabled) glEnable(GL_DEPTH_CLAMP); else glDisable(GL_DEPTH_CLAMP);
}

void VolumetricCloud::Unload() {
    glDisable(GL_BLEND);
    glDeleteTextures(1, &lowFreqNoiseTex);
    glDeleteTextures(1, &highFreqNoiseTex);
    glDeleteTextures(1, &cloudMapTex);
    glDeleteTextures(1, &heightTex);

    lowFreqNoiseTex = 0;
    highFreqNoiseTex = 0;
    cloudMapTex = 0;
    heightTex = 0;
    cloudVolume = nullptr;
    loaded = false;
}

void VolumetricCloud::RenderEditor(Editor& editor) {
    editor.BeginVolumetricCloud(*this);
}
