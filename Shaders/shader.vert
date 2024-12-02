#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;

//for static uniform buffer
layout(binding = 0) uniform UboViewProjection
{
    mat4 projection;
    mat4 view;
} uboViewProjection;

//for dynamic uniform buffer 
// NOT IN USE LEFT FOR REFERENCE
layout(binding = 1) uniform UboModel
{
    mat4 model;
} uboModel;

layout(push_constant) uniform PushModel
{
    mat4 model;
} pushModel;

layout (location = 0) out vec3 fragColor;

void main()
{
    gl_Position = uboViewProjection.projection * uboViewProjection.view * pushModel.model * vec4(pos, 1.0f);
    fragColor = color;
}