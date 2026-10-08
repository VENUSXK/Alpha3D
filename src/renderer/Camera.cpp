// Camera.cpp
#include "core/opengl.h"

#include <GLFW/glfw3.h>

#include "renderer/Camera.h"
#include "core/Log.h"

#include "core/Time.h"

#include "core/Window.h"

void mouse_callback(GLFWwindow* glfw_window, float xpos, float ypos)
{
    Window* window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
    if (!window) {
        LOG_ERROR("No Window pointer from GLFW UserPointer!");
        return;
    }

    Camera* camera = window->GetCamera();
    if (!camera) {
        LOG_ERROR("No Camera pointer in Window!");
        return;
    }

    camera->ProcessMouseMovement(xpos, ypos);
}


void Camera::ProcessMouseMovement(float xpos, float ypos) {

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    this->yaw += xoffset;
    this->pitch += yoffset;

    if (this->pitch > 89.0f)
        this->pitch = 89.0f;
    if (this->pitch < -89.0f)
        this->pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    front.y = sin(glm::radians(pitch));
    front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
    this->direction = glm::normalize(front);
}

void Camera::ProcessEditorInput(Window* window, bool isViewportHovered) {
    if (isViewportHovered || GetMoving())
    {
        double xpos, ypos;
        glfwGetCursorPos(window->GetGLFWWindow(), &xpos, &ypos);
        ProcessMouseMovement((float)xpos, (float)ypos);
        ProcessInput(window);
    }
    else
    {
        ResetMouseState();
    }
}

// newView: glm::lookAt(xx)
void Camera::SetDirection(const glm::mat4 newView)
{
    view = newView;
}

void Camera::SetDirection(const glm::vec3& newDirection)
{
    const float directionLength = glm::length(newDirection);
    if (directionLength <= glm::epsilon<float>())
        return;

    direction = newDirection / directionLength;

    yaw = glm::degrees(std::atan2(direction.z, direction.x));
    pitch = glm::degrees(std::asin(glm::clamp(direction.y, -1.0f, 1.0f)));

    const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    right = glm::normalize(glm::cross(direction, worldUp));
    up = glm::normalize(glm::cross(right, direction));
    RebuildView();
}

void Camera::ProcessInput(Window* window)
{
    GLFWwindow * glfw_window = window->GetGLFWWindow();
    if (glfwGetMouseButton(glfw_window, GLFW_MOUSE_BUTTON_2) == GLFW_PRESS) {
        moving = true;

        float cameraSpeed = moveSpeed * Time::DeltaTime();
        if (glfwGetKey(glfw_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            cameraSpeed *= 2; // adjust accordingly
        }
        if (glfwGetKey(glfw_window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(glfw_window, GLFW_KEY_UP)) {
            position += cameraSpeed * direction;
        }
        if (glfwGetKey(glfw_window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(glfw_window, GLFW_KEY_DOWN)) {
            position -= cameraSpeed * direction;
        }
        if (glfwGetKey(glfw_window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(glfw_window, GLFW_KEY_LEFT)) {
            position -= glm::normalize(glm::cross(direction, up)) * cameraSpeed;
        }
        if (glfwGetKey(glfw_window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(glfw_window, GLFW_KEY_RIGHT)) {
            position += glm::normalize(glm::cross(direction, up)) * cameraSpeed;
        }
        if (glfwGetKey(glfw_window, GLFW_KEY_Q) == GLFW_PRESS) {
            position -= glm::vec3(0.0f, 1.0f, 0.0f) * cameraSpeed;
        }
        if (glfwGetKey(glfw_window, GLFW_KEY_E) == GLFW_PRESS) {
            position += glm::vec3(0.0f, 1.0f, 0.0f) * cameraSpeed;
        }
        glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        position.y = glm::max(position.y, minHeight);
        RebuildView();
    }
    else {
        moving = false;

        glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        this->firstMouse = true;
    }

    this->direction = glm::normalize(direction);

    glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);
    this->right = glm::normalize(glm::cross(this->direction, world_up));
    this->up = glm::normalize(glm::cross(this->right, this->direction));
    RebuildView();
}

void Camera::RebuildView()
{
    glm::mat4 rotationView = glm::lookAt(
        glm::vec3(0.0f),
        direction,
        up
    );

    glm::mat4 translation = glm::translate(
        glm::mat4(1.0f),
        -position
    );

    view = rotationView * translation;
}

Camera::Camera(float fov, Window& window, glm::vec3 position, glm::vec3 target, float minHeight) {

    float aspectRatio = window.GetAspectRatio();
    const float window_width = static_cast<float>(window.GetWidth()), window_height = static_cast<float>(window.GetHeight());

    //glfwSetCursorPosCallback(window.GetGLFWWindow(), mouse_callback);

    m_Fov         = fov;
    m_AspectRatio = aspectRatio;

    this->position = position;
    this->minHeight = minHeight;
    glm::vec3 front = glm::normalize(target);
    this->direction = front;
    this->yaw = glm::degrees(atan2(front.z, front.x));
    this->pitch = glm::degrees(asin(front.y));

    glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);
    right = glm::normalize(glm::cross(direction, world_up));
    up    = glm::normalize(glm::cross(right, direction));

    this->lastX = window_width / 2;
    this->lastY = window_height / 2;

    view = glm::lookAt(position, position + direction, up);
    RebuildProjection();
}


void Camera::RebuildProjection() {
    if (m_IsOrtho)
        projection = glm::ortho(
            -m_OrthoSize * m_AspectRatio,  m_OrthoSize * m_AspectRatio,
            -m_OrthoSize,                   m_OrthoSize,
            m_Near, m_Far);
    else
        // projection = glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_Near, m_Far);
        projection = glm::infinitePerspective(glm::radians(m_Fov), m_AspectRatio, m_Near);
}

void Camera::SetProjection(glm::mat4 newProjection) {
    projection = newProjection;
}

void Camera::SetProjection(float fov, float aspectRatio) {
    m_Fov = fov;
    m_AspectRatio = aspectRatio;
    RebuildProjection();
}

void Camera::SetFov(float fov) {
    m_Fov = fov;
    RebuildProjection();
}

void Camera::SetOrtho(bool isOrtho) {
    m_IsOrtho = isOrtho;
    RebuildProjection();
}

void Camera::ResetMouseState() {
    firstMouse = true;
}
