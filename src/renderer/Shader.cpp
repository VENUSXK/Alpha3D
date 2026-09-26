#include "renderer/Shader.h"

std::string ReadFromShader(const char* shaderpath) {
    std::string shaderCode;
    std::ifstream shaderFile;
    shaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        shaderFile.open(shaderpath);
        std::stringstream shaderStream;
        shaderStream << shaderFile.rdbuf();
        shaderFile.close();
        shaderCode = shaderStream.str();
        LOG_INFO("Shader readed: {}.", shaderpath);
    }
    catch (std::ifstream::failure& e)
    {
        LOG_ERROR("Shader reading failed: {} {}", shaderpath, e.what());
    }
    return shaderCode;
}

struct shaderId {
    unsigned int vShaderId;
    unsigned int fShaderId;
};

unsigned int CompileShader(const char* shaderCode, const char* shaderpath, GLenum shaderType) {
    char infoLog[512];
    int success;
    unsigned int shaderId = glCreateShader(shaderType);
    glShaderSource(shaderId, 1, &shaderCode, NULL);
    glCompileShader(shaderId);
    // compile error
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shaderId, 512, NULL, infoLog);
        LOG_ERROR("Shader compiled failed: {}. {}", shaderpath, infoLog);
    }
    else {
        LOG_INFO("Shader compiled: {}.", shaderpath);
    }
    return shaderId;
}

unsigned int LinkShader(
    unsigned int vShaderId, unsigned int fShaderId,
    const char* vShaderPath, const char* fShaderPath
) {
    char infoLog[512];
    int success;
    unsigned int programId = glCreateProgram();
    glAttachShader(programId, vShaderId);
    glAttachShader(programId, fShaderId);
    glLinkProgram(programId);
    glGetProgramiv(programId, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programId, 512, NULL, infoLog);
        LOG_ERROR("Shaders linking failed: {}, {}. {}", vShaderPath, fShaderPath, infoLog);
    }
    else {
        LOG_INFO("Shader linked: {} and {}.", vShaderPath, fShaderPath);
    }
    glDeleteShader(vShaderId);
    glDeleteShader(fShaderId);
    return programId;
}

Shader::Shader(const char* vShaderPath, const char* fShaderPath)
{
    // get vertex
    std::string vShaderCode_str = ReadFromShader(vShaderPath);
    std::string fShaderCode_str = ReadFromShader(fShaderPath);

    const char* vShaderCode = vShaderCode_str.c_str();
    const char* fShaderCode = fShaderCode_str.c_str();

    // vertex shader compiling
    unsigned int vShaderId = CompileShader(vShaderCode, vShaderPath, GL_VERTEX_SHADER);
    unsigned int fShaderId = CompileShader(fShaderCode, fShaderPath, GL_FRAGMENT_SHADER);

    ID = LinkShader(vShaderId, fShaderId, vShaderPath, fShaderPath);
};

void Shader::use()
{
    glUseProgram(ID);
}

