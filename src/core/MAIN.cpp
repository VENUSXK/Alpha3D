
#define _CRT_SECURE_NO_WARNINGS
#define STBI_MSC_SECURE_CRT

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "core/opengl.h"

#include "core/Window.h"
#include "core/Viewport.h"
#include "core/Config.h"
#include "core/Log.h"
#include "core/Editor.h"

#include "scene/Scene.h"

#include "core/Time.h"
#include "renderer/Camera.h"
#include "utils/RenderProfiler.h"


int main()
{
    Log::Init();

    Config::Load("config/engine.toml");
    const auto& config = Config::Get();

    Window window;
    window.Init(config.window.width, config.window.height, config.window.monitorCenterX,
        config.window.monitorCenterY, config.window.hasMonitorCenter, config.window.title);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        LOG_ERROR("Failed to initialize GLAD");
        return -1;
    }
    glViewport(0, 0, config.window.width, config.window.height);

    Viewport viewport;
    viewport.Init(static_cast<float>(config.window.width), static_cast<float>(config.window.height));

    Editor editor;
    editor.Init(&window);

    Scene scene;

    RenderProfiler profiler;
    {
        A3_PROFILE_PASS(profiler, "One-time/Initial Scene Load");
        scene.Load(window);
    }

    Camera camera(config.renderer.fov, window, glm::vec3(-0.0f, 450.0f, -0.0f), glm::vec3(0.0f, 0.0f, 1.0f), 384.0f);
    window.SetCamera(&camera);

    window.SetResizeCallback([&](int width, int height) {
        if (height > 0) camera.SetProjection(config.renderer.fov, static_cast<float>(width) / static_cast<float>(height));
    });

    while (!window.ShouldClose())
    {
        profiler.BeginFrame();
        Time::Update();
        editor.Update(window);
        editor.BeginFrame(viewport);
        camera.SetProjection(camera.GetFov(), viewport.GetWidth() / viewport.GetHeight());

        // Viewport
        {
            A3_PROFILE_PASS(profiler, "Viewport");
            viewport.BeginRender();
            camera.ProcessEditorInput(&window, editor.IsViewportHovered());
            scene.Render(camera, profiler);
            viewport.EndRender();
        }

        // Editor
        {
            A3_PROFILE_PASS(profiler, "Editor UI");
            editor.BeginPerformance(profiler);
            editor.BeginCamera(camera);
            scene.RenderEditor(editor, camera);
            if (editor.HasPerformanceAffectingEdit()) profiler.ResetFrameHistory();
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
