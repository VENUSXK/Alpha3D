// renderer/Texture.h
#pragma once
#include <array>

#include <string>
#include <assimp/types.h>

#include "core/opengl.h"
#include "core/Log.h"

class DefaultTextures {
public:
    static unsigned int Black() {
        static unsigned int id = Create({ 0, 0, 0, 255 });
        return id;
    }
    static unsigned int White() {
        static unsigned int id = Create({ 255, 255, 255, 255 });
        return id;
    }
    static unsigned int Normal() {
        static unsigned int id = Create({ 128, 128, 255, 255 });
        return id;
    }
private:
    static unsigned int Create(std::array<unsigned char, 4> color) {
        unsigned int id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        return id;
    }
};

class Texture {
public:
    Texture(const std::string&);
    Texture(aiString, const std::string&, const std::string&);
    Texture(unsigned int id, const std::string& type) // 新增
        : mId(id), mType(type) {
    }

    ~Texture();

    unsigned int GetId() const { return mId; }
    aiString GetTexName() const { return mName; }
    const std::string& GetType() const { return mType; }
    static bool Load2D(unsigned int& texture, const std::string& path, unsigned int internalFormat = GL_RGBA8);
    static bool Load3D(unsigned int& texture, unsigned int size, const std::string& pathTemplate);
    unsigned int LoadHDR(const std::string& path);
    void Bind(unsigned int slot = 0);
private:
    unsigned int mId;
    int width = 128, height = 128;
    std::string mType;
    aiString mName;
};
