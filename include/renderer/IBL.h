#pragma once
#include "core/opengl.h"
#include <string>
#include <vector>

class Shader;
class Model;

class IBL {
public:
    unsigned int envCubemap = 0;
    unsigned int irradianceMap = 0;
    unsigned int prefilterMap = 0;
    unsigned int brdfLUTTexture = 0;

    void Load(const std::string& hdr_path, Model& cubeModel,
        Shader& to_cubemap_shader,
        Shader& irradiance_shader,
        Shader& prefilter_shader,
        Shader& brdf_integrate_shader
    );

    void Bind(Shader& shader) const;

    ~IBL();
private:
    std::vector<std::string> paths;
    std::vector<std::string> names;
    int selected = 0;
    bool changed = false;

    unsigned int captureFBO = 0;
    unsigned int captureRBO = 0;
};
