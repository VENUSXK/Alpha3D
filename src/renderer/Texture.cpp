// renderer/Texture.cpp
#pragma once

#include "renderer/Texture.h"

#include <cstdio>

#include "stb_image.h"


unsigned int TextureFromFile(const char* texName, const std::string& dirPath)
{
    std::string targetPath = dirPath + '/' + std::string(texName);

    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(targetPath.c_str(), &width, &height, &nrComponents, 0);
    if (data)
    {

        GLenum format1 = GL_RGB, format2 = GL_RGB;
        GLenum repeat = GL_REPEAT;
        if (nrComponents == 1)
            format1 = GL_RED, format2 = GL_RED;
        else if (nrComponents == 3)
            format1 = GL_RGB, format2 = GL_RGB;
        else if (nrComponents == 4)
            format1 = GL_RGBA, format2 = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format1, width, height, 0, format2, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        LOG_INFO("Texture loaded at path {}", targetPath);
        stbi_image_free(data);
    }
    else
    {
        LOG_ERROR("Texture {} failed to load at path {}", texName, targetPath);
        stbi_image_free(data);
    }

    return textureID;
}

Texture::Texture(aiString tex_name, const std::string& directory, const std::string& typeName) {
    this->mId = TextureFromFile(tex_name.C_Str(), directory);
    this->mType = typeName;
    this->mName = tex_name.C_Str();
}

Texture::Texture(const std::string& path) {

    glGenTextures(1, &mId);
    glBindTexture(GL_TEXTURE_2D, mId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        LOG_INFO("Texture loaded: {}", path);
    }
    else {
        LOG_ERROR("Failed to load texture: {}", path);
    }
    stbi_image_free(data);
}

Texture::~Texture() {
    //glDeleteTextures(1, &mId);
}

void Texture::Bind(unsigned int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, mId);
}

bool Texture::Load2D(unsigned int& texture, const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
        LOG_ERROR("Failed to load texture: {}", path);
        return false;
    }

    if (!texture) glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return true;
}

bool Texture::Load3D(unsigned int& texture, unsigned int size, const std::string& pathTemplate) {
    if (!texture) glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_3D, texture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, size, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    for (unsigned int layer = 0; layer < size; ++layer) {
        char path[512] = {};
        std::snprintf(path, sizeof(path), pathTemplate.c_str(), layer);

        int width = 0;
        int height = 0;
        int channels = 0;
        unsigned char* data = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);
        if (!data || width != static_cast<int>(size) || height != static_cast<int>(size)) {
            LOG_ERROR("Failed to load 3D texture layer: {}", path);
            stbi_image_free(data);
            glBindTexture(GL_TEXTURE_3D, 0);
            glDeleteTextures(1, &texture);
            texture = 0;
            return false;
        }

        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, layer, size, size, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    glGenerateMipmap(GL_TEXTURE_3D);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
    glBindTexture(GL_TEXTURE_3D, 0);
    return true;
}
