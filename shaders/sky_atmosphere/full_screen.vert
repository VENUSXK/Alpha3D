#version 330 core
const vec2 screen_triangle_vertices[3] = vec2[](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0,  3.0));
out vec2 screenUV;

void main() {
    vec2 pos = screen_triangle_vertices[gl_VertexID];
    gl_Position = vec4(pos, 0.0, 1.0);
    screenUV = pos * 0.5 + 0.5;
}
