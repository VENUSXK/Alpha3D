#pragma once

#include <string>

class LUT {
public:
    LUT(std::string name, int width, int height);
    ~LUT();

    LUT(const LUT&) = delete;
    LUT& operator=(const LUT&) = delete;
    LUT(LUT&& other) noexcept;
    LUT& operator=(LUT&& other) noexcept;

    const std::string& GetName() const { return name; }
    unsigned int GetTexture() const { return texture; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    void Allocate();
    void Bind(unsigned int slot = 0) const;
    bool Save(const std::string& path) const;

private:
    std::string name;
    unsigned int texture = 0;
    int width = 0;
    int height = 0;
};
