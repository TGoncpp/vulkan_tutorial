#version 450


layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    const vec3 lightDirection = normalize(vec3(0.0, -1.0, 1.0));

    // Calculate the dot product between the normal and the light direction
    float diff = max(dot(fragNormal, lightDirection), 0.2);

    // Simple diffuse lighting
    vec3 diffuse = diff * texture(texSampler,fragTexCoord ).xyz; // Assuming white light

    // Output color
    outColor =  vec4(diffuse,1.0);
    //outColor = vec4(fragColor, 1.0);

}