#include "scene/GameObject.h"
#include "renderer/Model.h"
#include "renderer/Shader.h"
#include "core/time.h"

void GameObject::Draw(Shader& overrideShader) const {
    if (!visible || !model) return;
    overrideShader.use();
    overrideShader.setMat4("model", transform.GetModelMatrix());
    model->Draw(overrideShader);
}

GameObject::GameObject(uint32_t id, std::string name, Model* model, Shader* shader)
    : id(id), name(std::move(name)), shader(shader), model(model) {
}

void GameObject::Draw() const {
    if (!visible || !shader || !model) return;

    shader->use();
    shader->setMat4("model", transform.GetModelMatrix());
    shader->setMat3("normalMatrix", transform.GetNormalMatrix());
    model->Draw(*shader);
}


void GameObject::Update()
{
    transform.Rotate(
        glm::vec3(
            rotate_speed_x * Time::DeltaTime(), 
            rotate_speed_y * Time::DeltaTime(), 
            rotate_speed_z * Time::DeltaTime()
        )
    );
    transform.SyncToMatrix();
}
