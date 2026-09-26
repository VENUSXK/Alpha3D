#pragma once
#include <cstddef>
#include <vector>
#include <glm/glm.hpp>

#include "renderer/Texture.h"


struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
};

class Shader;

class Mesh {
public:
    Mesh(const float* vertices, std::size_t size);
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures); // for assimp
    ~Mesh();

    // 禁用拷贝
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // 允许移动
    Mesh(Mesh&& other) noexcept
        : vao(other.vao), vbo(other.vbo), ebo(other.ebo),
        vertices(std::move(other.vertices)),
        indices(std::move(other.indices)),
        textures(std::move(other.textures)),
        vertex_count(other.vertex_count)
    {
        other.vao = 0;  // 置0，析构时不会删除
        other.vbo = 0;
        other.ebo = 0;
    }
    void FillMissingTextures();

    void Bind();
    
    void Draw();
    void Draw(Shader& shader);
private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    std::size_t vertex_count = 0;
};
