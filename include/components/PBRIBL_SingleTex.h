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

class PBRIBL_SingleTex : public PBRIBL_Base
{
public:

    PBRIBL_SingleTex(std::string path, std::string inst_name);
    void Load(Window& window);
    void Render(Camera& camera, RenderProfiler& profiler);
    void Unload();
    void RenderEditor(Editor& editor);

    GameObject* GetMainGameObject() { return game_object; }

    std::string GetName() const { return name; }

private:
    std::string name = "PBRIBLScene3";

    std::string model_path;
    Model model;

    GameObject* game_object = nullptr;
};
