#include "components/SkyAtmosphere.h"
#include "core/Editor.h"
#include "core/Log.h"
#include "core/opengl.h"
#include "renderer/Camera.h"
#include "utils/RenderProfiler.h"


void SetAtmosphereUniforms(const Shader& shader, const SkyAtmosphereParams& parameters) {
    shader.setBool("useRayleigh", parameters.useRayleigh);
    shader.setBool("useMie", parameters.useMie);
    shader.setBool("useAbsorption", parameters.useAbsorption);

    shader.setFloat("planetRadius", parameters.planetRadius);
    shader.setFloat("atmosphereRadius", parameters.atmosphereRadius);

    shader.setVec3("rayleighBeta", parameters.rayleighBeta);
    shader.setVec3("mieBeta", parameters.mieBeta);
    shader.setVec3("absorptionBeta", parameters.absorptionBeta);

    shader.setFloat("rayleighHeight", parameters.rayleighHeight);
    shader.setFloat("mieHeight", parameters.mieHeight);
    shader.setFloat("absorptionHeight", parameters.absorptionHeight);
    shader.setFloat("absorptionFalloff", parameters.absorptionFalloff);
}

void SetScatteringUniforms(const Shader& shader, const SkyAtmosphereParams& parameters) {
    shader.setBool("useTransmittanceLUT", parameters.useTransmittanceLUT);
    shader.setBool("useSkyViewLUT", parameters.useSkyViewLUT);
    shader.setInt("transmittanceLUT", 0);
    shader.setInt("skyViewLUT", 1);

    shader.setFloat("lightIntensity", parameters.lightIntensity);
    shader.setVec3("lightDirection", glm::normalize(parameters.lightDirection));

    shader.setFloat("cameraHeight", parameters.cameraHeight);
    shader.setFloat("mieG", parameters.mieG);

    shader.setFloat("exposure", parameters.exposure);
    shader.setFloat("gamma", parameters.gamma);

    shader.setInt("primarySteps", parameters.primarySteps);
    shader.setInt("lightSteps", parameters.lightSteps);
}

void SkyAtmosphere::AllocateAerialPerspectiveVolume() {
    const int size = glm::max(parameters.aerialPerspectiveSize, 1);
    if (aerialPerspectiveTexture && allocatedAerialPerspectiveSize == size) return;

    if (!aerialPerspectiveTexture) glGenTextures(1, &aerialPerspectiveTexture);
    if (!aerialPerspectiveFramebuffer) glGenFramebuffers(1, &aerialPerspectiveFramebuffer);

    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_3D, &previousTexture);

    glBindTexture(GL_TEXTURE_3D, aerialPerspectiveTexture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA16F, size, size, size, 0, GL_RGBA, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_3D, previousTexture);
    allocatedAerialPerspectiveSize = size;
}

void SkyAtmosphere::RenderAerialPerspectiveVolume(Camera& camera) {
    AllocateAerialPerspectiveVolume();

    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    GLint previousProgram = 0;
    GLint previousActiveTexture = 0;
    GLint previousViewport[4] = {};

    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    const GLboolean previousDepthTest = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean previousBlend = glIsEnabled(GL_BLEND);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, aerialPerspectiveFramebuffer);
    glDrawBuffer(GL_COLOR_ATTACHMENT0);
    glViewport(0, 0, allocatedAerialPerspectiveSize, allocatedAerialPerspectiveSize);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    shader_sky_atmosphere.use();
    SetAtmosphereUniforms(shader_sky_atmosphere, parameters);
    SetScatteringUniforms(shader_sky_atmosphere, parameters);

    shader_sky_atmosphere.setMat4("invProjection", glm::inverse(camera.GetProjection()));
    shader_sky_atmosphere.setMat4("invView", glm::inverse(camera.GetView()));
    shader_sky_atmosphere.setVec3("cameraPos", camera.GetPosition());

    shader_sky_atmosphere.setInt("MODE", 3);
    shader_sky_atmosphere.setInt("aerialPerspectiveSliceCount", allocatedAerialPerspectiveSize);
    shader_sky_atmosphere.setFloat("aerialPerspectiveMaxDistance", parameters.aerialPerspectiveMaxDistance);
    if (const LUT* lut = GetLUT("transmittance_lut.png")) lut->Bind(0);

    shader_sky_atmosphere.setBool("useVolumeShadow", shadowTexture != 0);
    shader_sky_atmosphere.setMat4("shadowViewProjection", shadowViewProjection);
    shader_sky_atmosphere.setInt("shadowMap", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glActiveTexture(GL_TEXTURE0);

    for (int slice = 0; slice < allocatedAerialPerspectiveSize; ++slice) {
        glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, aerialPerspectiveTexture, 0, slice);
        if (slice == 0 && glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            LOG_ERROR("Aerial perspective framebuffer is incomplete");
            break;
        }

        shader_sky_atmosphere.setInt("sliceId", slice);
        RenderFullscreenTriangle();
    }

    shader_sky_atmosphere.setInt("MODE", 0);
    shader_sky_atmosphere.setInt("aerialPerspectiveSteps", parameters.aerialPerspectiveSteps);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    if (previousDepthTest) glEnable(GL_DEPTH_TEST);
    if (previousBlend) glEnable(GL_BLEND);

    glActiveTexture(previousActiveTexture);
    glUseProgram(previousProgram);
}

void SkyAtmosphere::BindAerialPerspectiveVolume(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_3D, aerialPerspectiveTexture);
}

void SkyAtmosphere::Load(Window&) {
    luts.clear();
    luts.reserve(2);
    luts.emplace_back("transmittance_lut.png", 256, 64);
    luts.emplace_back("sky_view_lut.png", 192, 108);

    shader_sky_atmosphere.use();
    SetAtmosphereUniforms(shader_sky_atmosphere, parameters);
    SetScatteringUniforms(shader_sky_atmosphere, parameters);

    RenderTransmittanceLUT();
    SaveTransmittanceLUT("transmittance_lut.png");
    RenderSkyViewLUT();
    SaveSkyViewLUT("sky_view_lut.png");
    parametersDirty = false;
}

void SkyAtmosphere::Render(Camera& camera, RenderProfiler& profiler, bool outputLinear) {
    A3_PROFILE_PASS(profiler, "Atmosphere");
    shader_sky_atmosphere.use();
    shader_sky_atmosphere.setBool("outputLinear", outputLinear);

    shader_sky_atmosphere.setMat4("invProjection", glm::inverse(camera.GetProjection()));
    shader_sky_atmosphere.setMat4("invView", glm::inverse(camera.GetView()));
    shader_sky_atmosphere.setVec3("cameraPos", camera.GetPosition());
    shader_sky_atmosphere.setInt("MODE", 0);

    if (parametersDirty) {
        SetAtmosphereUniforms(shader_sky_atmosphere, parameters);
        SetScatteringUniforms(shader_sky_atmosphere, parameters);
        RenderTransmittanceLUT();
        RenderSkyViewLUT();
        shader_sky_atmosphere.setInt("MODE", 0);
        parametersDirty = false;
    }

    if (const LUT* transmittanceLUT = GetLUT("transmittance_lut.png")) transmittanceLUT->Bind(0);
    if (const LUT* skyViewLUT = GetLUT("sky_view_lut.png")) skyViewLUT->Bind(1);
    glActiveTexture(GL_TEXTURE0);

    shader_sky_atmosphere.setBool("useVolumeShadow", shadowTexture != 0 && !outputLinear);
    shader_sky_atmosphere.setMat4("shadowViewProjection", shadowViewProjection);
    shader_sky_atmosphere.setInt("shadowMap", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, shadowTexture);
    glActiveTexture(GL_TEXTURE0);

    RenderFullscreenTriangle();
}

void SkyAtmosphere::RenderTransmittanceLUT() {
    LUT* lut = GetLUT("transmittance_lut.png");
    if (!lut) return;
    RenderLUT(*lut, shader_sky_atmosphere, [&](Shader& shader) {
        shader.setInt("MODE", 1);
        shader.setInt("transmittanceLUTSteps", parameters.transmittanceLUTSteps);
    });
}

void SkyAtmosphere::RenderSkyViewLUT() {
    if (const LUT* transmittanceLUT = GetLUT("transmittance_lut.png")) transmittanceLUT->Bind(0);
    LUT* lut = GetLUT("sky_view_lut.png");
    if (lut) RenderLUT(*lut, shader_sky_atmosphere, [&](Shader& shader) { shader.setInt("MODE", 2); });
    glActiveTexture(GL_TEXTURE0);
}

bool SkyAtmosphere::SaveTransmittanceLUT(const std::string& outputPath) const {
    const LUT* lut = GetLUT("transmittance_lut.png");
    return lut && lut->Save("./generated/LUT/" + outputPath);
}

bool SkyAtmosphere::SaveSkyViewLUT(const std::string& outputPath) const {
    const LUT* lut = GetLUT("sky_view_lut.png");
    return lut && lut->Save("./generated/LUT/" + outputPath);
}

void SkyAtmosphere::Unload() {
    luts.clear();
    if (lut_framebuffer) glDeleteFramebuffers(1, &lut_framebuffer);
    lut_framebuffer = 0;

    if (aerialPerspectiveTexture) glDeleteTextures(1, &aerialPerspectiveTexture);
    if (aerialPerspectiveFramebuffer) glDeleteFramebuffers(1, &aerialPerspectiveFramebuffer);
    aerialPerspectiveTexture = 0;
    aerialPerspectiveFramebuffer = 0;
    allocatedAerialPerspectiveSize = 0;
}

void SkyAtmosphere::RenderEditor(Editor& editor) {
    editor.BeginSkyAtmosphere(*this);
}

LUT* SkyAtmosphere::GetLUT(const std::string& name) {
    for (LUT& lut : luts) if (lut.GetName() == name) return &lut;
    return nullptr;
}

const LUT* SkyAtmosphere::GetLUT(const std::string& name) const {
    for (const LUT& lut : luts) if (lut.GetName() == name) return &lut;
    return nullptr;
}

bool SkyAtmosphere::RenderLUT(LUT& lut, Shader& shader, const std::function<void(Shader&)>& configure) {
    lut.Allocate();
    if (!lut_framebuffer) glGenFramebuffers(1, &lut_framebuffer);

    GLint previousFramebuffer = 0;
    GLint previousViewport[4] = {};
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glBindFramebuffer(GL_FRAMEBUFFER, lut_framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lut.GetTexture(), 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("LUT framebuffer is incomplete: {}", lut.GetName());

        glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
        return false;
    }

    glViewport(0, 0, lut.GetWidth(), lut.GetHeight());
    shader.use();
    configure(shader);
    RenderFullscreenTriangle();

    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    return true;
}

void SkyAtmosphere::RenderFullscreenTriangle() {
    static unsigned int vertexArray = 0;
    if (!vertexArray) glGenVertexArrays(1, &vertexArray);
    glBindVertexArray(vertexArray);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}
