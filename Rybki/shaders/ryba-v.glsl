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
uniform float swimSpeed;
uniform float randomOffset;

void main() {
    vec3 localPos = aPos;

    float waveSpeed = 5.0 * swimSpeed; // szybkość
    float waveStrength = 0.15;         // moc
    float waveFrequency = 5.0;         // ilość zgięć

    float wave = sin(time * waveSpeed + localPos.y * waveFrequency + randomOffset);
    float tailFactor = clamp(localPos.y, 0.0, 1.0); 
    
    localPos.x += wave * waveStrength * tailFactor;
    FragPos = vec3(model * vec4(localPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;  
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}