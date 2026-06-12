#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float randomOffset;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    float heightFromBottom = max(worldPos.y - (-1.1), 0.0);
    float swayFactor = smoothstep(0.1, 0.8, heightFromBottom);

    float swayX = sin(time * 1.2 + randomOffset + worldPos.z * 0.5) * 0.02 * swayFactor;
    float swayZ = sin(time * 1.0 + randomOffset + worldPos.x * 0.5 + 1.0) * 0.015 * swayFactor;

    worldPos.x += swayX;
    worldPos.z += swayZ;

    worldPos.x += swayX;
    worldPos.z += swayZ;

    FragPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * worldPos;
}