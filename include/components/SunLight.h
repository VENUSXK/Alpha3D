#pragma once

#include <glm/glm.hpp>

class LUT;

class SunLight {
public:
    SunLight();
    float longitude = 100.0f;
    float latitude = 33.4f;
    glm::vec3 direction = glm::vec3(0.0f);
    glm::vec3 baseColor = glm::vec3(1.0f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 10.0f;

    void UpdateDirection();
    void MarkColorDirty() { colorDirty = true; }
    void UpdateColorFromTransmittanceLUT(const LUT& transmittanceLUT, float observerHeight, float planetRadius, float atmosphereRadius);
private:
    bool colorDirty = true;
};
