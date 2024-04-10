#version 450

layout(binding = 0) uniform uniformBufferObject
{
    mat4 model;
    mat4 view;
    mat4 proj;
}
ubo;
layout(push_constant) uniform pushConstant
{
    mat4 model;
}
ps;

layout (location = 0) in vec2 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;


void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model * ps.model * vec4(inPosition, 0.0, 1.0);
    vec4 tNormal =  ps.model * vec4(inNormal,0);
    fragNormal = normalize(tNormal.xyz); // interpolation of normal attribute in fragment shader.
    fragTexCoord = inTexCoord;
}