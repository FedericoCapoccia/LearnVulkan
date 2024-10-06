#version 450

layout (location = 0) in vec3 positions;
layout (location = 1) in vec3 colors;

layout (location = 0) out vec3 frag_color;

void main() {
    gl_Position = vec4(positions, 1.0f);
    frag_color = colors;
}