#pragma once

#include "renderer/Shader.h"

class PostProcess {
public:
    float exposure = 0.0f;
    float gamma = 2.2f;

    ~PostProcess();
    void Render(unsigned int hdrTexture);
private:
    unsigned int fullscreenVao = 0;
    Shader shader{ "shaders/post_process/full_screen.vert", "shaders/post_process/post_process.frag" };
};
