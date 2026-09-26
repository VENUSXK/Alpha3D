#pragma once
#include <string>
#include <cstdint>
#include <optional>
#include "utils/Transform.h"

class Model;
class Shader;

struct LightComponent {
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 5.0f;
};

struct PBRTestComponent {
    glm::vec3 albedo = glm::vec3(0.5f, 0.5f, 0.5f);
    float roughness = 0.1f;
    float metallic = 1.0f;
};

class GameObject {
public:
    float rotate_speed_x;
    float rotate_speed_y;
    float rotate_speed_z;

    std::optional<LightComponent> light;
    std::optional<PBRTestComponent> pbr_sphere;

    GameObject(uint32_t id, std::string name, Model* model, Shader* shader);

    uint32_t GetID()   const { return id; }
    const std::string& GetName() const { return name; }
    void SetName(const std::string& name) { this->name = name; }

    // Transform
    Transform& GetTransform() { return transform; }
    const Transform& GetTransform() const { return transform; }

    void SetPosition(const glm::vec3& pos) { transform.SetPosition(pos); }
    void SetRotation(const glm::vec3& rot) { transform.SetRotation(rot); }
    void SetScale(const float scale) { transform.SetScale(glm::vec3(scale)); }

    void Translate(const glm::vec3& delta) { transform.Translate(delta); }
    void Rotate(const glm::vec3& deltaDegrees) { transform.Rotate(deltaDegrees); }
    void Scale(const glm::vec3& factor) { transform.Scale(factor); }

    glm::vec3 GetPosition() const { return transform.position; }
    glm::vec3 GetRotation() const { return transform.rotation; }
    glm::vec3 GetScale()    const { return transform.scale; }

    void ResetTransform() { transform.Reset(); }

    // Shader
    Shader* GetShader() const { return shader; }
    void SetShader(Shader* shader) { this->shader = shader; }

    // Visibility
    bool IsVisible() const { return visible; }
    void SetVisible(bool v) { visible = v; }
    void ToggleVisible() { visible = !visible; }

    // Draw
    void Draw() const;

    void Update();
private:


    uint32_t id;
    std::string name;

    Transform transform;
    
    Shader* shader = nullptr;
    Model* model = nullptr;
    
    bool visible = true;
};
