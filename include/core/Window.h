#pragma once
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <glfw/glfw3.h>
#include <string>
#include <functional>

class Camera;

class Window {
public:
    void Init(int width, int height, int monitorCenterX, int monitorCenterY, bool hasMonitorCenter, const std::string& title);
    ~Window();

    void Update();
    void Destroy();

    int GetWidth() const;
    int GetHeight() const;

    bool ShouldClose();
    void ProcessKeyboardInput();
    float GetAspectRatio() const;

    GLFWwindow* GetGLFWWindow();
    Camera* GetCamera() const { return camera; }
    void SetCamera(Camera* camera) { this->camera = camera; }

    void SetResizeCallback(std::function<void(int, int)> callback);
    void SetScaleCallback(std::function<void(float)> callback);

    float GetDeltaTime() const { return delta_time; }
    GLFWmonitor* GetCurrentMonitor() const { return currentMonitor; }
    float GetCurrentScale() const { return currentScale; }
    int GetCurrentMonitorIndex() const { return currentMonitorIndex; }

private:
    static GLFWmonitor* FindMonitorContainingPoint(int x, int y, int* monitorIndex = nullptr);
    void UpdateCurrentMonitor();
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    float delta_time = 0.0f;
    double last_time = 0.0f;

    GLFWwindow* glfw_window = nullptr;
    int width = 0, height = 0;
    std::string title;
    Camera* camera = nullptr;

    GLFWmonitor* currentMonitor = nullptr;
    int currentMonitorIndex = -1;
    float currentScale = 1.0f;

    std::function<void(int, int)> ResizeCallback;
    std::function<void(float)> ScaleCallback;

};
