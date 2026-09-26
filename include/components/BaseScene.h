// components/BaseScene.h
#pragma once
#include <functional>
#include <string>
#include <vector>
#include <algorithm>

#include "scene/Scene.h"
#include "renderer/Model.h"
#include "core/Log.h"

class Camera;
class Editor;
class RenderProfiler;
class Window;

class GameObject;
class BaseScene
{
public:
    virtual ~BaseScene() = default;

    virtual void Load(Window& window);
    virtual void Render(Camera& camera, RenderProfiler& profiler) = 0;
    virtual void Unload() = 0;

    virtual void RenderEditor(Editor& editor);
    virtual GameObject* GetMainGameObject() { return nullptr; };

    virtual std::string GetName() const = 0;
    void SetDeltaTime(float dt) { delta_time = dt; }
    void MarkParametersDirty() { parametersDirty = true; }
    
    void RenderFullscreenTriangle();

    bool LoadVolumeTex(unsigned int& texture, unsigned int texSize, std::string pathTemplate);
    bool LoadTex(unsigned int& texture, std::string path);
    
    void RenderLUT(GLuint lutTexture, int width, int height, GLuint framebuffer, Shader& shader, std::function<void(Shader&)> setUniforms);
    bool SaveTextureLUT(GLuint texture, int width, int height, const std::string& fileName) const;
    

protected:
    float delta_time = 0.0f;
    bool parametersDirty = true;
    Scene scene;


    
    glm::vec3 ambient = glm::vec3(0.2f);

    Model cubeModel = Model::Cube();
    Model sphereModel = Model::Sphere(128, 64);
};
