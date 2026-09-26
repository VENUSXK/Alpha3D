// components/BaseScene.cpp
#include "core/OpenGL.h"
#include <stb_image.h>


#include "core/Editor.h"
#include "core/Log.h"

#include "components/BaseScene.h"
#include "renderer/Shader.h"










// path_template "assets/textures/nubis2017.%03d.tga"
bool BaseScene::LoadVolumeTex(unsigned int& texture, unsigned int texSize, std::string pathTemplate) {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_3D, texture);

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, texSize, texSize, texSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    for (unsigned int z = 0; z < texSize; ++z) {
        char path[256];
        snprintf(path, sizeof(path), pathTemplate.c_str(), z + 1);

        int width = 0, height = 0, channels = 0;
        unsigned char* slice = stbi_load(path, &width, &height, &channels, 4);

        if (!slice || width != texSize || height != texSize) {
            stbi_image_free(slice);
            glBindTexture(GL_TEXTURE_3D, 0);
            LOG_ERROR("Reading volumetric cloud texture {} failed!", z + 1);
            return false;
        }

        glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, z, texSize, texSize, 1, GL_RGBA, GL_UNSIGNED_BYTE, slice);

        stbi_image_free(slice);
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
