
#define _CRT_SECURE_NO_WARNINGS
#define STBI_MSC_SECURE_CRT

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "core/opengl.h"

#include "core/Window.h"
#include "core/Config.h"
#include "core/Log.h"
#include "core/Editor.h"

#include "scene/Scene.h"

#include "core/Time.h"
#include "core/Viewport.h"
#include "renderer/Camera.h"
#include "renderer/Capture.h"
#include "renderer/PostProcess.h"
#include "utils/RenderProfiler.h"


int main()
{
    Log::Init();

    Config::Load("config/engine.toml");
    const auto& config = Config::Get();

    Window window;
    window.Init(config.window.width, config.window.height, config.window.monitorCenterX,
        config.window.monitorCenterY, config.window.hasMonitorCenter, config.window.title);
    if (!window.GetGLFWWindow()) return -1;

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOG_ERROR("Failed to initialize GLAD");
        return -1;
    }
    if (!GLAD_GL_VERSION_4_6) {
        LOG_ERROR("OpenGL 4.6 is not supported by the current GPU driver");
        return -1;
    }
    LOG_INFO("OpenGL version: {}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    LOG_INFO("OpenGL renderer: {}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    Editor editor;
    editor.Init(&window);

    Scene scene;

    RenderProfiler profiler;
    {
        A3_PROFILE_PASS(profiler, "One-time/Initial Scene Load");
        scene.Load(window);
    }

    Camera camera(config.renderer.fov, window, glm::vec3(-0.0f, 10.0f, -0.0f), glm::vec3(0.0f, 0.0f, -1.0f), 0.0f);
    window.SetCamera(&camera);
    Viewport hdrViewport;
    hdrViewport.Init(static_cast<float>(config.window.width), static_cast<float>(config.window.height));
    PostProcess postProcess;
    Capture capture;

    while (!window.ShouldClose())
    {
        profiler.BeginFrame();
        Time::Update();
        editor.Update(window);
        editor.BeginFrame();
        int framebufferWidth = 0, framebufferHeight = 0;
        glfwGetFramebufferSize(window.GetGLFWWindow(), &framebufferWidth, &framebufferHeight);
        camera.ProcessEditorInput(&window, !editor.Hover() && !editor.WantCaptureKeyboard());
        if (framebufferWidth > 0 && framebufferHeight > 0) {
            A3_PROFILE_PASS(profiler, "Scene");
            camera.SetProjection(camera.GetFov(), static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight));
            hdrViewport.Resize(static_cast<float>(framebufferWidth), static_cast<float>(framebufferHeight));
            hdrViewport.BeginRender();
            scene.Render(camera, profiler);
            hdrViewport.EndRender();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, framebufferWidth, framebufferHeight);
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            postProcess.Render(hdrViewport.GetColorTexture());
            if (capture.HasPendingRequest()) capture.CaptureFrame(scene, camera, postProcess, profiler, framebufferWidth, framebufferHeight);
        }

        // Editor
        {
            A3_PROFILE_PASS(profiler, "Editor UI");
            ImGui::SetNextWindowSize(ImVec2(520.0f, 720.0f), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Editor")) {
                editor.BeginPerformance(profiler);
                editor.BeginCamera(camera);
                editor.BeginPostProcess(postProcess);
                editor.BeginCapture(capture);
                scene.RenderEditor(editor, camera);
                if (editor.HasPerformanceAffectingEdit()) profiler.ResetFrameHistory();
            }
            ImGui::End();
            editor.EndFrame();
        }

        {
            A3_PROFILE_PASS(profiler, "Present");
            window.Update();
        }
        profiler.EndFrame();
    }

    scene.Unload();
    profiler.Shutdown();

    return 0;
}
