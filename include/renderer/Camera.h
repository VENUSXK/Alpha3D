#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Window;

class Camera {
public:
    Camera(float fov, Window& window, glm::vec3 position, glm::vec3 target, float minHeight);

    bool SetView();

    void SetProjection(float fov, float aspectRatio);

    bool GetMoving() { return this->moving; }
    void SetMoving(bool moving) { this->moving = moving; }

    float GetFov()    const { return m_Fov; }
    bool  IsOrtho()   const { return m_IsOrtho; }
    void  SetFov(float fov);
    void  SetOrtho(bool isOrtho);

    void SetPosition(const glm::vec3& position) {
        this->position = position;
        this->view = glm::lookAt(this->position, this->position + this->direction, this->up);
    }
    void ResetMouseState();

    glm::vec3 GetPosition() { return position; }
    glm::vec3 GetDirection() const { return direction; }
    void SetDirection(const glm::vec3& direction);
    void SetDirection(const glm::mat4 direction);
    glm::mat4 GetView() { return view; }
    glm::mat4 GetProjection() { return projection; }
    void SetProjection(glm::mat4 newProjection);

    void ProcessInput(Window* window);
    void ProcessMouseMovement(float xpos, float ypos);
    void ProcessEditorInput(Window* window, bool isViewportHovered);

    void RebuildView();

    float moveSpeed = 10.0f;

private:

    float minHeight = 1.8f;
    float deltaTime = 0.0f;	// Time between current frame and last frame
    float lastFrame = 0.0f; // Time of last frame

    void RebuildProjection();

    // camera direction
    glm::vec3 position;
    glm::vec3 direction;

    glm::vec3 up;
    glm::vec3 right;

    // MVP properties
    glm::mat4 view;
    glm::mat4 projection;

    float m_Fov         = 90.0f;
    float m_AspectRatio = 1.0f;
    float m_Near        = 0.1f;
    float m_Far = 3000.0f;
    bool  m_IsOrtho     = false;
    float m_OrthoSize   = 5.0f;

    float yaw = -90.0f;
    float pitch = 0.0f;
    float lastX;
    float lastY;

    bool moving = false;
    bool firstMouse = true;
};
