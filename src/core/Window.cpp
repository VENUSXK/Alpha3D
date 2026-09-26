#include <windows.h>
#include "core/opengl.h"
#include "core/Window.h"
#include "core/Log.h"

#include <cmath>
#include <imm.h>

#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "imm32.lib")

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include "core/Config.h"
#include "imgui.h"
#include <shobjidl.h>
#include "stb_image.h"

void Window::SetResizeCallback(std::function<void(int, int)> callback) {
    ResizeCallback = callback;
}

void Window::SetScaleCallback(std::function<void(float)> callback) {
    ScaleCallback = callback;
}

void Window::framebuffer_size_callback(GLFWwindow* glfw_window, int width, int height)
{
    if (width <= 0 || height <= 0) return;

    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    if (!window) {
        LOG_ERROR("GLFW UserPointer get failed!");
        return;
    }

    window->width = width;
    window->height = height;

    if (window->ResizeCallback) window->ResizeCallback(width, height);
    glViewport(0, 0, window->width, window->height);
}

void processInput(Window* window)
{
    GLFWwindow* glfw_window = window->GetGLFWWindow();
    if (glfwGetKey(glfw_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(glfw_window, true);
}

GLFWmonitor* Window::FindMonitorContainingPoint(int x, int y, int* monitorIndex)
{
    if (monitorIndex) {
        *monitorIndex = -1;
    }

    int monitorCount = 0;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);

    for (int i = 0; i < monitorCount; ++i) {
        GLFWmonitor* monitor = monitors[i];

        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(monitor, &monitorX, &monitorY);

        const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
        if (!videoMode) {
            continue;
        }

        const int monitorRight = monitorX + videoMode->width;
        const int monitorBottom = monitorY + videoMode->height;

        if (x >= monitorX && x < monitorRight &&
            y >= monitorY && y < monitorBottom) {
            if (monitorIndex) {
                *monitorIndex = i;
            }
            return monitor;
        }
    }

    return nullptr;
}

void Window::Init(int width, int height, int monitorCenterX, int monitorCenterY, bool hasMonitorCenter, const std::string& title) {

    // set window properties
    
    glfwInit();

    this->width = width;
    this->height = height;
    this->title = title;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);

    //glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
    //glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);
    //glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    
    // GLFW init
    glfw_window = glfwCreateWindow(this->width, this->height, this->title.c_str(), NULL, NULL);
    if (glfw_window == NULL)
    {
        LOG_ERROR("Window creation failed: {}, {}x{}!", title, this->width, this->height);
        glfwTerminate();
        return;
    }

    GLFWmonitor* targetMonitor = nullptr;
    if (hasMonitorCenter) {
        targetMonitor = FindMonitorContainingPoint(monitorCenterX, monitorCenterY);
    }
    if (!targetMonitor) {
        targetMonitor = glfwGetPrimaryMonitor();
    }

    if (targetMonitor) {
        int workX = 0;
        int workY = 0;
        int workWidth = 0;
        int workHeight = 0;
        glfwGetMonitorWorkarea(targetMonitor, &workX, &workY, &workWidth, &workHeight);

        const int windowX = workX + (workWidth > this->width ? (workWidth - this->width) / 2 : 0);
        const int windowY = workY + (workHeight > this->height ? (workHeight - this->height) / 2 : 0);
        glfwSetWindowPos(glfw_window, windowX, windowY);
    }

    glfwMakeContextCurrent(glfw_window);

    glfwSetFramebufferSizeCallback(glfw_window, framebuffer_size_callback);
    LOG_INFO("Window created: {}, {}x{}.", title, this->width, this->height);

    #ifdef _WIN32
        HWND hwnd = glfwGetWin32Window(glfw_window);
        ImmAssociateContextEx(hwnd, NULL, IACE_IGNORENOCONTEXT);

        // 自定义标题栏背景色（Windows 11 / Win10 较新版本）
        COLORREF color = RGB(28, 28, 28); // 深灰色
        DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &color, sizeof(color));

        // 自定义边框颜色
        COLORREF border_color = RGB(28, 28, 28);
        DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &border_color, sizeof(border_color));

        // 暗色模式（让标题栏文字变白）
        BOOL dark = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
    #endif

    GLFWimage images[1];

    int icon_width, icon_height, channels;
    unsigned char* data = stbi_load("assets/icon.png", &icon_width, &icon_height, &channels, 4);

    images[0].width = icon_width;
    images[0].height = icon_height;
    images[0].pixels = data;

    glfwSetWindowIcon(glfw_window, 1, images);

    stbi_image_free(data);

    glfwSetWindowUserPointer(glfw_window, this);

    UpdateCurrentMonitor();

}

void Window::Destroy()
{
    if (!glfw_window) {
        return;
    }

    int windowX = 0;
    int windowY = 0;
    int windowWidth = 0;
    int windowHeight = 0;
    glfwGetWindowPos(glfw_window, &windowX, &windowY);
    glfwGetWindowSize(glfw_window, &windowWidth, &windowHeight);

    const int windowCenterX = windowX + windowWidth / 2;
    const int windowCenterY = windowY + windowHeight / 2;
    GLFWmonitor* lastMonitor = FindMonitorContainingPoint(windowCenterX, windowCenterY);

    if (lastMonitor) {
        int monitorX = 0;
        int monitorY = 0;
        glfwGetMonitorPos(lastMonitor, &monitorX, &monitorY);

        const GLFWvidmode* videoMode = glfwGetVideoMode(lastMonitor);
        if (videoMode) {
            WindowConfig& windowConfig = Config::Get().window;
            windowConfig.monitorCenterX = monitorX + videoMode->width / 2;
            windowConfig.monitorCenterY = monitorY + videoMode->height / 2;
            windowConfig.hasMonitorCenter = true;

            LOG_INFO(
                "Saving monitor center ({}, {}).",
                windowConfig.monitorCenterX,
                windowConfig.monitorCenterY
            );
        }
    }

    Config::Save("config/engine.toml");

    glfwDestroyWindow(glfw_window);
    glfw_window = nullptr;
    currentMonitor = nullptr;
    currentMonitorIndex = -1;
    glfwTerminate();

    LOG_INFO("Window destroyed.");
}

Window::~Window()
{
    Destroy();
}

GLFWwindow* Window::GetGLFWWindow() {
    return this->glfw_window;
}

int Window::GetWidth() const {
    return this->width;
}

int Window::GetHeight() const {
    return this->height;
}

void Window::Update() {
    double current_time = glfwGetTime();
    delta_time = (float)(current_time - last_time);
    last_time = current_time;

    glfwPollEvents();
    UpdateCurrentMonitor();
    glfwSwapBuffers(this->glfw_window);
}

bool Window::ShouldClose() {
    return glfwWindowShouldClose(this->glfw_window);
}

void Window::ProcessKeyboardInput() {
    if (glfwGetKey(this->glfw_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(this->glfw_window, true);
}

float Window::GetAspectRatio() const {
    return height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 0.0f;
}

void Window::UpdateCurrentMonitor()
{
    if (!glfw_window) {
        return;
    }

    int windowX = 0;
    int windowY = 0;
    int windowWidth = 0;
    int windowHeight = 0;

    // 这里必须使用窗口尺寸，而不是 framebuffer 尺寸。
    // 在高 DPI 屏幕上，两者可能不同。
    glfwGetWindowPos(glfw_window, &windowX, &windowY);
    glfwGetWindowSize(glfw_window, &windowWidth, &windowHeight);

    if (windowWidth <= 0 || windowHeight <= 0) {
        return;
    }

    const int centerX = windowX + windowWidth / 2;
    const int centerY = windowY + windowHeight / 2;

    int detectedMonitorIndex = -1;
    GLFWmonitor* detectedMonitor = FindMonitorContainingPoint(
        centerX,
        centerY,
        &detectedMonitorIndex
    );

    float detectedScale = currentScale;

    if (detectedMonitor) {
        float xScale = 1.0f;
        float yScale = 1.0f;

        glfwGetMonitorContentScale(detectedMonitor, &xScale, &yScale);
        detectedScale = xScale;
    }

    const bool monitorChanged = detectedMonitor != currentMonitor;
    const bool scaleChanged = std::abs(detectedScale - currentScale) > 0.001f;


    if (!monitorChanged && !scaleChanged) {
        return;
    }

    currentMonitor = detectedMonitor;
    currentMonitorIndex = detectedMonitorIndex;
    currentScale = detectedScale;

    if (currentMonitor) {
        const char* monitorName = glfwGetMonitorName(currentMonitor);

        LOG_INFO("Window center is on monitor {}: {}, scale {}", currentMonitorIndex, monitorName ? monitorName : "Unknown", currentScale);
    }
    else {
        LOG_INFO("Window center is outside all monitors");
    }

    // 只有检测到有效显示器时才通知缩放变化。
    if (currentMonitor && ScaleCallback) {
        ScaleCallback(currentScale);
    }
}
