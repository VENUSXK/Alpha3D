#include "renderer/Model.h"

#include <cmath>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <algorithm>
#include <stdexcept>
#include <utility>
#include "stb_image.h"

Model Model::Cube()
{
    const glm::vec3 normals[] = {
        { 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f },
        { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }
    };

    const glm::vec2 uvs[] = {
        { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f }
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    vertices.reserve(24);
    indices.reserve(36);

    for (const glm::vec3& normal : normals) {
        glm::vec3 reference = normal.y != 0.0f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 tangent = glm::normalize(glm::cross(reference, normal));
        glm::vec3 bitangent = glm::cross(normal, tangent);

        unsigned int base = static_cast<unsigned int>(vertices.size());

        for (const glm::vec2& uv : uvs) {
            Vertex vertex{};
            vertex.Position = normal * 0.5f + tangent * (uv.x - 0.5f) + bitangent * (uv.y - 0.5f);
            vertex.Normal = normal;
            vertex.TexCoords = uv;
            vertex.Tangent = tangent;

            vertices.push_back(vertex);
        }

        indices.insert(indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
    }

    Model model;
    model.mMeshes.emplace_back(std::move(vertices), std::move(indices), std::move(textures));

    return model;
}

Model Model::Sphere(int sectorCount, int stackCount, float radius) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    constexpr float pi = 3.14159265358979323846f;
    const float sectorStep = 2.0f * pi / sectorCount;
    const float stackStep = pi / stackCount;

    for (int stack = 0; stack <= stackCount; ++stack) {
        const float stackAngle = pi / 2.0f - stack * stackStep;
        const float xy = radius * std::cos(stackAngle);
        const float z = radius * std::sin(stackAngle);

        for (int sector = 0; sector <= sectorCount; ++sector) {
            const float sectorAngle = sector * sectorStep;
            const float x = xy * std::cos(sectorAngle);
            const float y = xy * std::sin(sectorAngle);
            vertices.push_back({
                { x, y, z },
                { x / radius, y / radius, z / radius },
                { static_cast<float>(sector) / sectorCount, static_cast<float>(stack) / stackCount },
                { 0.0f, 0.0f, 0.0f }
            });
        }
    }

    const int stride = sectorCount + 1;
    for (int stack = 0; stack < stackCount; ++stack) {
        for (int sector = 0; sector < sectorCount; ++sector) {
            const unsigned int first = stack * stride + sector;
            const unsigned int next = first + stride;
            if (stack != 0) indices.insert(indices.end(), { first, next, first + 1 });
            if (stack != stackCount - 1) indices.insert(indices.end(), { first + 1, next, next + 1 });
        }
    }

    Model model;
    model.mMeshes.emplace_back(std::move(vertices), std::move(indices), std::vector<Texture>{});
    return model;
}

Model Model::Terrain(const char* heightTexPath, float width, float depth, float heightScale, std::vector<Texture> textures)
{
    if (width <= 0.0f || depth <= 0.0f || heightScale < 0.0f)
        throw std::runtime_error("Invalid terrain dimensions.");

    int columns = 0, rows = 0, channels = 0;
    stbi_us* heights = stbi_load_16(heightTexPath, &columns, &rows, &channels, 1);

    if (!heights || columns < 2 || rows < 2) {
        stbi_image_free(heights);
        throw std::runtime_error("Failed to load terrain heightmap.");
    }

    std::vector<Vertex> vertices(static_cast<size_t>(columns) * rows);
    std::vector<unsigned int> indices;
    indices.reserve(static_cast<size_t>(columns - 1) * (rows - 1) * 6);

    // Generate one vertex per pixel, with the Y-axis representing height.
    size_t centerIndex = static_cast<size_t>(rows / 2) * columns + columns / 2;
    float centerHeight = heights[centerIndex] / 65535.0f * heightScale;
    for (int z = 0; z < rows; ++z) {
        for (int x = 0; x < columns; ++x) {
            size_t index = static_cast<size_t>(z) * columns + x;
            float u = static_cast<float>(x) / (columns - 1);
            float v = static_cast<float>(z) / (rows - 1);
            float height = heights[index] / 65535.0f * heightScale - centerHeight - 2.0f;

            vertices[index].Position = glm::vec3((u - 0.5f) * width, height, (v - 0.5f) * depth);
            vertices[index].TexCoords = glm::vec2(u, 1.0f - v);
            vertices[index].Normal = glm::vec3(0.0f);
            vertices[index].Tangent = glm::vec3(0.0f);
        }
    }

    stbi_image_free(heights);

    // 每四个相邻顶点组成两个朝上的三角形。
    for (int z = 0; z < rows - 1; ++z) {
        for (int x = 0; x < columns - 1; ++x) {
            unsigned int a = static_cast<unsigned int>(static_cast<size_t>(z) * columns + x);
            unsigned int b = a + 1, c = a + columns, d = c + 1;

            indices.insert(indices.end(), { a, c, b, b, c, d });
        }
    }

    // 累加相邻三角形的法线，得到平滑地形。
    for (size_t i = 0; i < indices.size(); i += 3) {
        Vertex& a = vertices[indices[i]];
        Vertex& b = vertices[indices[i + 1]];
        Vertex& c = vertices[indices[i + 2]];

        glm::vec3 normal = glm::cross(b.Position - a.Position, c.Position - a.Position);
        a.Normal += normal;
        b.Normal += normal;
        c.Normal += normal;
    }

    // 根据横向地形变化计算切线，并使其垂直于法线。
    for (int z = 0; z < rows; ++z) {
        for (int x = 0; x < columns; ++x) {
            size_t row = static_cast<size_t>(z) * columns;
            Vertex& vertex = vertices[row + x];

            int left = std::max(x - 1, 0), right = std::min(x + 1, columns - 1);
            glm::vec3 tangent = vertices[row + right].Position - vertices[row + left].Position;

            vertex.Normal = glm::normalize(vertex.Normal);
            vertex.Tangent = glm::normalize(tangent - vertex.Normal * glm::dot(vertex.Normal, tangent));
        }
    }

    Model model;
    model.mMeshes.emplace_back(std::move(vertices), std::move(indices), std::move(textures));
    model.mMeshes.back().FillMissingTextures();

    return model;
}

void Model::Draw(Shader& shader)
{
    for (unsigned int i = 0; i < mMeshes.size(); i++)
        mMeshes[i].Draw(shader);
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString tex_name;
        mat->GetTexture(type, i, &tex_name);
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            if (std::strcmp(textures_loaded[j].GetTexName().C_Str(), tex_name.C_Str()) == 0)
            {
                textures.push_back(std::move(textures_loaded[j]));
                skip = true;
                break;
            }
        }

        if (!skip)
        {
            LOG_INFO("Loading model texture '{}'", std::string(tex_name.C_Str()));

            Texture texture(tex_name, this->directory, typeName);

            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
        else {
            LOG_INFO("Texture exists '{}'", std::string(tex_name.C_Str()));

        }
    }
    return textures;
}


Mesh Model::processMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
        for (int t = 0; t < AI_TEXTURE_TYPE_MAX; t++) {
            int count = material->GetTextureCount((aiTextureType)t);
            if (count > 0) {
                aiString path;
                material->GetTexture((aiTextureType)t, 0, &path);
                LOG_INFO("Consists texture type: {}: {}", t, path.C_Str());
            }
        }
    }

    // vao vertex processing
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;

        glm::vec3 vector;

        // setting vertex position
        vector.x = mesh->mVertices[i].x;
        vector.y = mesh->mVertices[i].y;
        vector.z = mesh->mVertices[i].z;

        vertex.Position = vector;

        // setting vertex normal
        vector.x = mesh->mNormals[i].x;
        vector.y = mesh->mNormals[i].y;
        vector.z = mesh->mNormals[i].z;

        vertex.Normal = vector;

        // setting vertex texture coords
        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
        }
        else {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }

        if (mesh->mTangents) {
            vertex.Tangent = { mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z };
        }

        // adding vertex
        vertices.push_back(vertex);
    }

    // ebo indices processing
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
            
    }

    // material processing
    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<Texture> albedoMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "albedo");
        textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());

        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "normal");
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        std::vector<Texture> roughnessMap = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "arm");
        textures.insert(textures.end(), roughnessMap.begin(), roughnessMap.end());

        std::vector<Texture> emissiveMap = loadMaterialTextures(material, aiTextureType_EMISSIVE, "emissive");
        textures.insert(textures.end(), emissiveMap.begin(), emissiveMap.end());
    }
    //return Mesh(vertices, indices, textures);
    return Mesh(vertices, indices, std::move(textures));
}

void Model::processNode(aiNode* node, const aiScene* scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        Mesh processed_mesh = processMesh(mesh, scene);
        processed_mesh.FillMissingTextures();
        this->mMeshes.push_back(std::move(processed_mesh));
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

Model::Model(const char* modelPath)
{
    const std::string path = modelPath;
    Assimp::Importer import;
    const aiScene* scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_ERROR(import.GetErrorString());
        return;
    }
    else {
        LOG_INFO("Model loaded: {}, meshes: {}", path, scene->mNumMeshes);
    }
    directory = path.substr(0, path.find_last_of('/'));

    processNode(scene->mRootNode, scene);
}
