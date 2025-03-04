#version 450
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoordinates;

layout(set = 1, binding = 0) uniform sampler2D textureSampler;

layout(location = 0) out vec4 result;

void main()
{
    result = texture(textureSampler, fragTexCoordinates);
    //result = vec4(fragColor, 1.0f);
}