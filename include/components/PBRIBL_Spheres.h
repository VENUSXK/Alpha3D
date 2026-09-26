#pragma once
#include "renderer/Mesh.h"
#include "renderer/Model.h"
#include "renderer/Shader.h"
#include "renderer/Camera.h"
#include "renderer/IBL.h"
#include "scene/Scene.h"
#include "components/BaseScene.h"
#include "components/PBRIBL_Base.h"

class Window;

class PBRIBL_Spheres : public PBRIBL_Base
{
public:

    void Load(Window& window);
    void Render(Camera& camera, RenderProfiler& profiler);
    void Unload();
    void RenderEditor(Editor& editor);

    //GameObject* GetMainGameObject() { return sphere; }

    std::string GetName() const { return name; }

private:
    std::string name = "PBRIBL_Spheres";

    glm::vec3 sphere_albedo = glm::vec3(0.5f, 0.5f, 0.5f);
    GameObject* sphere = nullptr;

};
