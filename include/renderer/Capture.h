#pragma once

#include <string>

#include "core/Viewport.h"

class Camera;
class PostProcess;
class RenderProfiler;
class Scene;

class Capture {
public:
    int width = 1920;
    int height = 1080;
    bool showFrame = false;
    char outputDirectory[512] = "./captures/";
    char fileName[256] = "capture.png";

    Capture() = default;
    ~Capture();
    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;

    void Request() { captureRequested = true; }
    bool HasPendingRequest() const { return captureRequested; }
    bool CaptureFrame(Scene& scene, Camera& camera, PostProcess& postProcess, RenderProfiler& profiler, int mainWidth, int mainHeight);
    const std::string& GetStatus() const { return status; }

private:
    bool EnsureTargets();
    bool SaveOutput();

    Viewport hdrViewport;
    bool hdrViewportInitialized = false;
    unsigned int outputFramebuffer = 0;
    unsigned int outputTexture = 0;
    int allocatedWidth = 0;
    int allocatedHeight = 0;
    bool captureRequested = false;
    std::string status;
};
