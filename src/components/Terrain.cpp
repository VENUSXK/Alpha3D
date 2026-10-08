#include "components/Terrain.h"
#include "components/SkyAtmosphere.h"
#include "core/Log.h"
#include "core/opengl.h"
#include "renderer/Camera.h"
#include "renderer/IBL.h"
#include "renderer/Texture.h"
#include "scene/GameObject.h"
#include "scene/Scene.h"
#include "utils/RenderProfiler.h"
#include <stb_image.h>
#include <string>

void Terrain::Load(Scene& scene) {
    stbi_set_flip_vertically_on_load(false);
    if (!Texture::Load2D(terrainColorTexture, "assets/terrain/terrain-color-map.png", GL_SRGB8_ALPHA8)) return;
    glBindTexture(GL_TEXTURE_2D, terrainColorTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    terrainModel = std::make_unique<Model>("assets/terrain/height-map.glb");
    terrainObject = &scene.AddGameObject("Terrain", terrainModel.get(), &terrainShader);
    terrainObject->SetPosition(glm::vec3(-4000.0f, 0.0f, 4000.0f));
    terrainObject->SetScale(1.0f);
}

void Terrain::BindTextures() const {
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, terrainColorTexture);
    glActiveTexture(GL_TEXTURE0);
}

void Terrain::Render(Camera& camera, const IBL& lighting, const SkyAtmosphere& atmosphere, RenderProfiler& profiler) {
    if (!terrainObject || !terrainColorTexture) return;
    A3_PROFILE_PASS(profiler, "Terrain");
    lighting.Bind(terrainShader);
    terrainShader.setMat4("view", camera.GetView());
    terrainShader.setMat4("projection", camera.GetProjection());
    terrainShader.setVec3("viewPos", camera.GetPosition());

    terrainShader.setInt("terrainColorMap", 7);
    terrainShader.setVec3("lightPos", parameters.lightPosition);
    terrainShader.setVec3("light.intensity", parameters.lightIntensity);
    terrainShader.setBool("useAerialPerspective", parameters.useAerialPerspective);

    GLint viewport[4] = {};
    glGetIntegerv(GL_VIEWPORT, viewport);
    terrainShader.setVec3("aerialPerspectiveViewportSize", glm::vec3(float(viewport[2]), float(viewport[3]), 0.0f));
    terrainShader.setFloat("aerialPerspectiveMaxDistance", atmosphere.parameters.aerialPerspectiveMaxDistance);
    terrainShader.setInt("aerialPerspectiveSliceCount", glm::max(atmosphere.parameters.aerialPerspectiveSize, 1));
    terrainShader.setInt("aerialPerspectiveVolume", 12);

    atmosphere.BindAerialPerspectiveVolume(12);
    BindTextures();
    terrainObject->Draw();
}


void Terrain::Unload() {
    glDeleteTextures(1, &terrainColorTexture);
    terrainColorTexture = 0;
    terrainObject = nullptr;
    terrainModel.reset();
}