#pragma once
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>
struct Transform {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
    glm::mat4 matrix = glm::mat4(1.0f);
    std::array<glm::vec3, 8> localCorners{};
    std::array<glm::vec3, 8> worldCorners{};
    void UpdateCorners() {
        for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
            worldCorners[cornerIndex] = glm::vec3(matrix * glm::vec4(localCorners[cornerIndex], 1.0f));
    }
    void Update() {
        matrix = glm::translate(glm::mat4(1.0f), position);
        matrix *= glm::mat4_cast(glm::quat(glm::radians(rotation)));
        matrix = glm::scale(matrix, scale);
        UpdateCorners();
    }
    void SyncFromMatrix() {
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::quat orientation;
        glm::decompose(matrix, scale, orientation, position, skew, perspective);
        rotation = glm::degrees(glm::eulerAngles(orientation));
        UpdateCorners();
    }
    const glm::mat4& GetModelMatrix() const { return matrix; }
    glm::mat3 GetNormalMatrix() const { return glm::mat3(glm::transpose(glm::inverse(matrix))); }
    void SetPosition(const glm::vec3& newPosition) { position = newPosition; Update(); }
    void SetRotation(const glm::vec3& newRotation) { rotation = newRotation; Update(); }
    void SetScale(const glm::vec3& newScale) { scale = newScale; Update(); }
    void Translate(const glm::vec3& delta) { position += delta; Update(); }
    void Rotate(const glm::vec3& deltaDegrees) { rotation += deltaDegrees; Update(); }
    void Scale(const glm::vec3& factor) { scale *= factor; Update(); }
    void Reset() {
        position = glm::vec3(0.0f);
        rotation = glm::vec3(0.0f);
        scale = glm::vec3(1.0f);
        Update();
    }
    bool IsIdGameObject() const {
        return position == glm::vec3(0.0f) && rotation == glm::vec3(0.0f) && scale == glm::vec3(1.0f);
    }
};
