#include "core/Viewport.h"
#include "core/Log.h"

void Viewport::Init(float w, float h) {
    width = w; height = h;

    // 1. 创建颜色贴图——渲染结果最终存在这里

    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width, (GLsizei)height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 2. 创建深度贴图——3D 场景必须有深度测试，否则前后遮挡会错乱
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // 3. 创建 FBO 本身，把上面两张贴图挂上去
    glGenFramebuffers(1, &fboID);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Viewport::BeginRender() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glBindFramebuffer(GL_FRAMEBUFFER, fboID);
    glViewport(0, 0, (int)width, (int)height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Viewport::EndRender() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// Viewport.cpp
void Viewport::Resize(float newWidth, float newHeight)
{
    if (newWidth == width && newHeight == height) return;   // 没变化就不用重建
    if (newWidth <= 0 || newHeight <= 0) return;             // 防止面板缩到 0 时出问题

    this->width = newWidth;
    this->height = newHeight;

    // 重新设置颜色贴图大小
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, (GLsizei)width, (GLsizei)height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

    // 重新设置深度贴图大小
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    // 不需要重新创建 FBO 本身，贴图尺寸变了之后 FBO 自动适配新尺寸
}