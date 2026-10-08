#include "renderer/IBL.h"

#include <filesystem>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"
#include "renderer/Camera.h"
#include "renderer/Model.h"
#include "renderer/Shader.h"
#include "utils/RenderProfiler.h"
#include "scene/Scene.h"

#include <algorithm>
#include <vector>

#include "stb_image_write.h"

static void RenderQuad_IBL()
{
    static unsigned int quadVAO = 0, quadVBO = 0;
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void IBL::Render(Scene& scene, Camera& camera, RenderProfiler& profiler)
{
    GLsizei captureSize = 1024;

    // STAGE 1  CAPTURING SCENE TO CUBE_MAP
    // initing FBOs
    if (!captureFBO) glGenFramebuffers(1, &captureFBO);
    if (!captureRBO) glGenRenderbuffers(1, &captureRBO);

    

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] = {
        glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0)),
    };

    // generating cubemap
    if (!envCubemap) glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, captureSize, captureSize, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // saving previous view settings
    Camera captureCamera = camera;
    captureCamera.SetPosition(glm::vec3(0.0f));
    captureCamera.SetProjection(captureProjection);
    GLint previousDrawFBO = 0;
    GLint previousReadFBO = 0;
    GLint previousViewport[4];

    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFBO);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFBO);
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    // binding frame buffer object and render buffer object
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, captureSize, captureSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // rendering frame to cube box
    bool captureComplete = true;
    glViewport(0, 0, captureSize, captureSize);
    for (unsigned int face = 0; face < 6; ++face) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, envCubemap, 0);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "IBL capture framebuffer incomplete\n";
            captureComplete = false;
            break;
        }

        captureCamera.SetDirection(captureViews[face]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene.RenderSkyLight(captureCamera, profiler, true);
    }

    // generarte mip maps
    if (captureComplete) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }

    // recovering viewports and FBO
    if (!captureComplete) {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFBO);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFBO);
        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, captureSize, captureSize);


    // STAGE 2  GENERATING IRRADIANCE MAPS
    // generating irradiance map
    if (!irradianceMap) glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    irradiance_shader.use();
    irradiance_shader.setInt("environmentMap", 0);
    irradiance_shader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, 32, 32);
    for (unsigned int i = 0; i < 6; ++i) {
        irradiance_shader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        cubeModel.Draw(irradiance_shader);
    }

    // ------------------ Prefilter map ------------------
    
    const int prefilter_resolution = 1024;
    const int maxMipLevels = 5;

    if (!prefilterMap) glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, prefilter_resolution, prefilter_resolution, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    prefilter_shader.use();
    prefilter_shader.setInt("environmentMap", 0);
    prefilter_shader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    for (int mip = 0; mip < maxMipLevels; ++mip) {
        const int mipSize = (unsigned int)(prefilter_resolution >> mip);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
        glViewport(0, 0, mipSize, mipSize);

        const float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);
        prefilter_shader.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i) {
            prefilter_shader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            cubeModel.Draw(prefilter_shader);
        }
    }


    // ------------------ BRDF LUT ------------------
    if (!brdfLUTTexture) glGenTextures(1, &brdfLUTTexture);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);
    glViewport(0, 0, 512, 512);
    brdf_integrate_shader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    RenderQuad_IBL();

    // recovering FBO
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFBO);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

    dirty = false;
}

void IBL::Bind(Shader& shader) const
{
    shader.use();
    shader.setInt("irradianceMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

    shader.setInt("prefilterMap", 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);

    shader.setInt("brdfLUT", 2);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
}

IBL::~IBL()
{
    if (envCubemap)     glDeleteTextures(1, &envCubemap);
    if (irradianceMap)  glDeleteTextures(1, &irradianceMap);
    if (prefilterMap)   glDeleteTextures(1, &prefilterMap);
    if (brdfLUTTexture) glDeleteTextures(1, &brdfLUTTexture);
    if (captureFBO)     glDeleteFramebuffers(1, &captureFBO);
    if (captureRBO)     glDeleteRenderbuffers(1, &captureRBO);
}
