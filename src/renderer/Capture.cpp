#include "renderer/Capture.h"

#include <algorithm>
#include <filesystem>
#include <system_error>
#include <vector>

#include <stb_image_write.h>

#include "core/Log.h"
#include "core/opengl.h"
#include "renderer/Camera.h"
#include "renderer/PostProcess.h"
#include "scene/Scene.h"
#include "utils/RenderProfiler.h"

Capture::~Capture() {
    if (outputFramebuffer) glDeleteFramebuffers(1, &outputFramebuffer);
    if (outputTexture) glDeleteTextures(1, &outputTexture);
}

bool Capture::EnsureTargets() {
    GLint maxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    width = std::clamp(width, 1, maxTextureSize); height = std::clamp(height, 1, maxTextureSize);
    if (!hdrViewportInitialized) { hdrViewport.Init(float(width), float(height)); hdrViewportInitialized = true; }
    else hdrViewport.Resize(float(width), float(height));
    if (!outputFramebuffer) glGenFramebuffers(1, &outputFramebuffer);
    if (!outputTexture) glGenTextures(1, &outputTexture);
    if (allocatedWidth == width && allocatedHeight == height) return true;
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, outputTexture, 0);
    GLenum statusCode = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (statusCode != GL_FRAMEBUFFER_COMPLETE) { status = "Capture framebuffer is incomplete"; LOG_ERROR("{}: 0x{:X}", status, statusCode); return false; }
    allocatedWidth = width; allocatedHeight = height;
    return true;
}

bool Capture::SaveOutput() {
    std::filesystem::path directory = outputDirectory[0] ? std::filesystem::path(outputDirectory) : std::filesystem::path("captures");
    std::filesystem::path outputPath = directory / (fileName[0] ? fileName : "capture.png");
    if (outputPath.extension() != ".png") outputPath.replace_extension(".png");
    std::error_code error;
    std::filesystem::create_directories(outputPath.parent_path(), error);
    if (error) { status = "Failed to create capture directory: " + error.message(); LOG_ERROR("{}", status); return false; }
    std::vector<unsigned char> pixels(size_t(width) * size_t(height) * 4);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    const size_t rowSize = size_t(width) * 4;
    for (int y = 0; y < height / 2; ++y) {
        auto top = pixels.begin() + size_t(y) * rowSize;
        auto bottom = pixels.begin() + size_t(height - 1 - y) * rowSize;
        std::swap_ranges(top, top + rowSize, bottom);
    }
    if (!stbi_write_png(outputPath.string().c_str(), width, height, 4, pixels.data(), width * 4)) {
        status = "Failed to save capture: " + outputPath.string(); LOG_ERROR("{}", status); return false;
    }
    status = "Saved: " + outputPath.string(); LOG_INFO("{}", status);
    return true;
}

bool Capture::CaptureFrame(Scene& scene, Camera& camera, PostProcess& postProcess, RenderProfiler& profiler, int mainWidth, int mainHeight) {
    captureRequested = false;
    if (mainWidth <= 0 || mainHeight <= 0) { status = "Capture failed: invalid main viewport size"; return false; }
    GLint previousDrawFramebuffer = 0, previousReadFramebuffer = 0, previousViewport[4] = {}, previousPackAlignment = 4;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFramebuffer); glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);
    glGetIntegerv(GL_VIEWPORT, previousViewport); glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
    if (!EnsureTargets()) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer); glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]); return false;
    }
    camera.SetProjection(camera.GetFov(), float(width) / float(height));
    hdrViewport.BeginRender(); scene.Render(camera, profiler); hdrViewport.EndRender();
    glBindFramebuffer(GL_FRAMEBUFFER, outputFramebuffer); glViewport(0, 0, width, height);
    postProcess.Render(hdrViewport.GetColorTexture());
    glBindFramebuffer(GL_READ_FRAMEBUFFER, outputFramebuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    bool saved = SaveOutput();
    glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
    camera.SetProjection(camera.GetFov(), float(mainWidth) / float(mainHeight));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFramebuffer); glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFramebuffer);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    return saved;
}
