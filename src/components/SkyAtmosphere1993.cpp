#include "components/SkyAtmosphere.h"
#include "utils/RenderProfiler.h"

void SkyAtmosphere1993::Load(Window& window)
{
    BaseScene::Load(window); // glEnable
}

void SkyAtmosphere1993::Render(Camera& camera, RenderProfiler&)
{
    shader_sky_atmosphere.use();
    shader_sky_atmosphere.setBool("useRayleigh", parameters.useRayleigh);
    shader_sky_atmosphere.setBool("useMie", parameters.useMie);
    shader_sky_atmosphere.setBool("useAbsorption", parameters.useAbsorption);
    shader_sky_atmosphere.setMat4("invProjection", glm::inverse(camera.GetProjection()));
    shader_sky_atmosphere.setMat4("invView", glm::inverse(camera.GetView()));
    shader_sky_atmosphere.setVec3("cameraPos", camera.GetPosition());
    shader_sky_atmosphere.setVec3("lightDirection", glm::normalize(parameters.lightDirection));
    shader_sky_atmosphere.setFloat("lightIntensity", parameters.lightIntensity);
    shader_sky_atmosphere.setFloat("cameraHeight", parameters.cameraHeight);
    shader_sky_atmosphere.setFloat("planetRadius", parameters.planetRadius);
    shader_sky_atmosphere.setFloat("atmosphereRadius", parameters.atmosphereRadius);
    cubeModel.Draw(shader_sky_atmosphere);
}

void SkyAtmosphere1993::Unload()
{
    scene.Clear();
}


void SkyAtmosphere1993::RenderEditor(Editor& editor)
{
    editor.BeginSkyAtmosphere(*this);
    BaseScene::RenderEditor(editor);
}
