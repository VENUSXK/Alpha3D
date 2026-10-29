#version 460 core

in vec2 screenUV;
out vec4 FragColor;

uniform sampler2D hdrSceneTexture;
uniform float exposure;
uniform float gamma;

void main() {
    vec3 color = texture(hdrSceneTexture, screenUV).rgb * exp2(exposure);
    color = color / (color + vec3(1.0));
    color = pow(max(color, vec3(0.0)), vec3(1.0 / max(gamma, 0.0001)));
    FragColor = vec4(color, 1.0);
}
