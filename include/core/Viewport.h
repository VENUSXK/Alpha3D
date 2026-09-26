#pragma once
#include "core/opengl.h"

class Camera;
class Viewport {
public:
    void Init(float width, float height);
    void Resize(float width, float height);
    void BeginRender();
    void EndRender();
    GLuint GetFBOId() const { return this->fboID; }
    GLuint GetColorTexture() const { return colorTexture; }
    float GetWidth() const { return width; }
    float GetHeight() const { return height; }

private:
    GLuint fboID = 0;
    GLuint colorTexture = 0;
    GLuint depthTexture = 0;
    float width = 0, height = 0;
};