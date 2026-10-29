#include "renderer/PostProcess.h"

#include "core/opengl.h"

PostProcess::~PostProcess() { if (fullscreenVao) glDeleteVertexArrays(1, &fullscreenVao); }

void PostProcess::Render(unsigned int hdrTexture) {
    if (!fullscreenVao) glGenVertexArrays(1, &fullscreenVao);
    const GLboolean depthTestEnabled = glIsEnabled(GL_DEPTH_TEST), blendEnabled = glIsEnabled(GL_BLEND), cullEnabled = glIsEnabled(GL_CULL_FACE);
    GLboolean depthWriteEnabled = GL_TRUE;
    GLint previousActiveTexture = GL_TEXTURE0;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWriteEnabled);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glDisable(GL_DEPTH_TEST); glDisable(GL_BLEND); glDisable(GL_CULL_FACE); glDepthMask(GL_FALSE);
    shader.use();
    shader.setInt("hdrSceneTexture", 0);
    shader.setFloat("exposure", exposure);
    shader.setFloat("gamma", gamma);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glBindVertexArray(fullscreenVao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    glActiveTexture(previousActiveTexture);
    glDepthMask(depthWriteEnabled);
    if (depthTestEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (blendEnabled) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (cullEnabled) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
}
