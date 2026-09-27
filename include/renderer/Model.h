#pragma once
#include <vector>
#include <string>

#include "renderer/Mesh.h"

struct aiMaterial;
struct aiNode;
struct aiScene;
struct aiMesh;

class Shader;

enum aiTextureType : int;

class Model
{
public:
    Model() = default;
    Model(const char* path);
    static Model Cube();
    static Model Sphere(int sectorCount = 36, int stackCount = 18, float radius = 0.5f);

    void Draw(Shader& shader);
private:
    std::vector<Mesh> mMeshes;
    std::string directory;

    std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);

    std::vector<Texture> textures_loaded;
};

