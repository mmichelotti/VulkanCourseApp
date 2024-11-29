#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;

layout(binding = 0) uniform MVP
{
    mat4 projection;
    mat4 view;
    mat4 model;
} mvp;

layout (location = 0) out vec3 fragColor;

void main()
{
    gl_Position = mvp.projection * mvp.view * mvp.model * vec4(pos, 1.0f);
    fragColor = color;
}