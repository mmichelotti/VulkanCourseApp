#version 450

layout (location = 0) in vec3 pos;
layout (location = 1) in vec3 color;
layout (location = 2) in vec2 texCoordinates;

//Buffer for STATIC UNIFORM
layout(set = 0, binding = 0) uniform UboViewProjection
{
    mat4 projection;
    mat4 view;
} uboViewProjection;

//non-BUFFER for DYNAMIC push const
layout(push_constant) uniform PushModel
{
    mat4 model;
} pushModel;

layout (location = 0) out vec3 fragColor;
layout (location = 1) out vec2 fragTexCoordinates;

void main()
{
    gl_Position = uboViewProjection.projection * uboViewProjection.view * pushModel.model * vec4(pos, 1.0f);
    fragColor = color;
    fragTexCoordinates = texCoordinates;
}