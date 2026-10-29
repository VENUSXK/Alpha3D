#include "components/SunLight.h"
#include "core/opengl.h"
#include "renderer/LUT.h"

static glm::vec2 TransmittanceParamsToUv(float viewRadius, float zenithCos, float planetRadius, float atmosphereRadius) {
    float atmosphereHeight = glm::sqrt(glm::max(atmosphereRadius * atmosphereRadius - planetRadius * planetRadius, 0.0f));
    float radiusHeight = glm::sqrt(glm::max(viewRadius * viewRadius - planetRadius * planetRadius, 0.0f));
    float discriminant = viewRadius * viewRadius * (zenithCos * zenithCos - 1.0f) + atmosphereRadius * atmosphereRadius;
    float distanceToTop = -viewRadius * zenithCos + glm::sqrt(glm::max(discriminant, 0.0f));
    float minimumDistance = atmosphereRadius - viewRadius;
    float maximumDistance = radiusHeight + atmosphereHeight;
    float horizontalUv = (distanceToTop - minimumDistance) / glm::max(maximumDistance - minimumDistance, 0.000001f);
    return glm::clamp(glm::vec2(horizontalUv, radiusHeight / glm::max(atmosphereHeight, 0.000001f)), glm::vec2(0.0f), glm::vec2(1.0f));
}

SunLight::SunLight() { UpdateDirection(); }

void SunLight::UpdateDirection() {
    latitude = glm::clamp(latitude, -90.0f, 90.0f);
    float longitudeRadians = glm::radians(longitude);
    float latitudeRadians = glm::radians(latitude);
    direction = glm::normalize(glm::vec3(cos(latitudeRadians) * cos(longitudeRadians), sin(latitudeRadians), cos(latitudeRadians) * sin(longitudeRadians)));
    MarkColorDirty();
}

void SunLight::UpdateColorFromTransmittanceLUT(const LUT& transmittanceLUT, float observerHeight, float planetRadius, float atmosphereRadius) {
    if (!colorDirty || !transmittanceLUT.GetTexture()) return;
    float viewRadius = glm::clamp(planetRadius + observerHeight, planetRadius + 1.0f, atmosphereRadius - 1.0f);
    float zenithCos = glm::clamp(glm::normalize(direction).y, -1.0f, 1.0f);
    float groundDiscriminant = viewRadius * viewRadius * (zenithCos * zenithCos - 1.0f) + planetRadius * planetRadius;
    if (zenithCos < 0.0f && groundDiscriminant >= 0.0f) { color = glm::vec3(0.0f); colorDirty = false; return; }
    glm::vec2 uv = TransmittanceParamsToUv(viewRadius, zenithCos, planetRadius, atmosphereRadius);
    int pixelX = glm::clamp(int(uv.x * float(transmittanceLUT.GetWidth() - 1) + 0.5f), 0, transmittanceLUT.GetWidth() - 1);
    int pixelY = glm::clamp(int(uv.y * float(transmittanceLUT.GetHeight() - 1) + 0.5f), 0, transmittanceLUT.GetHeight() - 1);
    float sampledColor[3] = {};
    glGetTextureSubImage(transmittanceLUT.GetTexture(), 0, pixelX, pixelY, 0, 1, 1, 1, GL_RGB, GL_FLOAT, sizeof(sampledColor), sampledColor);
    color = baseColor * glm::vec3(sampledColor[0], sampledColor[1], sampledColor[2]);
    colorDirty = false;
}
