#include "renderer/LUT.h"

#include <algorithm>
#include <filesystem>
#include <utility>
#include <vector>

#include "core/Log.h"
#include "core/opengl.h"
#include "stb_image_write.h"

LUT::LUT(std::string name, int width, int height)
    : name(std::move(name)), width(width), height(height) {
}

LUT::~LUT() {
    if (texture) glDeleteTextures(1, &texture);
}

LUT::LUT(LUT&& other) noexcept
    : name(std::move(other.name)), texture(other.texture), width(other.width), height(other.height) {
    other.texture = 0;
}

LUT& LUT::operator=(LUT&& other) noexcept {
    if (this == &other) return *this;
    if (texture) glDeleteTextures(1, &texture);

    name = std::move(other.name);
    texture = other.texture;
    width = other.width;
    height = other.height;
    other.texture = 0;
    return *this;
}

void LUT::Allocate() {
    if (!texture) glGenTextures(1, &texture);

    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, previousTexture);
}

void LUT::Bind(unsigned int slot) const {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture);
}

bool LUT::Save(const std::string& path) const {
    if (!texture) return false;

    std::vector<float> pixels(static_cast<size_t>(width) * height * 3);
    std::vector<unsigned char> image(pixels.size());
    GLint previousTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, pixels.data());
    glBindTexture(GL_TEXTURE_2D, previousTexture);

    for (int y = 0; y < height; ++y) {
        const int flippedY = height - 1 - y;
        for (int x = 0; x < width; ++x) {
            for (int channel = 0; channel < 3; ++channel) {
                const size_t source = (static_cast<size_t>(y) * width + x) * 3 + channel;
                const size_t target = (static_cast<size_t>(flippedY) * width + x) * 3 + channel;
                image[target] = static_cast<unsigned char>(std::clamp(pixels[source], 0.0f, 1.0f) * 255.0f);
            }
        }
    }

    const std::filesystem::path outputPath(path);
    if (!outputPath.parent_path().empty()) std::filesystem::create_directories(outputPath.parent_path());
    if (!stbi_write_png(path.c_str(), width, height, 3, image.data(), width * 3)) {
        LOG_ERROR("Failed to save LUT: {}", path);
        return false;
    }

    LOG_INFO("LUT saved: {}", path);
    return true;
}
