#include "scene/GameObject.h"
#include "renderer/Model.h"
#include "renderer/Shader.h"

void GameObject::Draw(Shader& overrideShader) const {
    if (!visible || !model) return;
    overrideShader.use();
    overrideShader.setMat4("model", transform.GetModelMatrix());
    model->Draw(overrideShader);
}

GameObject::GameObject(uint32_t id, std::string name, Model* model, Shader* shader)
    : id(id), name(std::move(name)), shader(shader), model(model) {
    if (!model) return;
    glm::vec3 modelCenter = model->GetCenter();
    glm::vec3 halfSize = model->GetSize() * 0.5f;
    for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
        glm::vec3 cornerSign((cornerIndex & 1) ? 1.0f : -1.0f, (cornerIndex & 2) ? 1.0f : -1.0f, (cornerIndex & 4) ? 1.0f : -1.0f);
        transform.localCorners[cornerIndex] = modelCenter + halfSize * cornerSign;
    }
    transform.Update();
}

void GameObject::Draw() const {
    if (!visible || !shader || !model) return;

    shader->use();
    shader->setMat4("model", transform.GetModelMatrix());
    shader->setMat3("normalMatrix", transform.GetNormalMatrix());
    if (pbr_custom_component) {
        shader->setVec3("baseColor", pbr_custom_component->albedo);
        shader->setFloat("roughness", pbr_custom_component->roughness);
        shader->setFloat("metallic", pbr_custom_component->metallic);
    }
    model->Draw(*shader);
}
