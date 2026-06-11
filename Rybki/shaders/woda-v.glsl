#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 Normal;
out vec3 FragPos;
out float isTopFace;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

void main()
{
    vec3 pos = aPos;
    
    isTopFace = (aNormal.y > 0.5) ? 1.0 : 0.0;
    
    float waveHeight = 0.01;
    float waveSpeed = 2.0;
    float waveFrequency = 3.0;
    
    vec3 modifiedNormal = aNormal;
    
    if (aPos.y > 0.79) {
        pos.y += sin(pos.x * waveFrequency + time * waveSpeed) * waveHeight;
        pos.y += cos(pos.z * waveFrequency + time * waveSpeed) * waveHeight;
        
        if (isTopFace > 0.5) {
            modifiedNormal.x -= cos(pos.x * waveFrequency + time * waveSpeed) * 0.15;
            modifiedNormal.z += sin(pos.z * waveFrequency + time * waveSpeed) * 0.15;
        }
    }

    FragPos = vec3(model * vec4(pos, 1.0));
    Normal = mat3(transpose(inverse(model))) * normalize(modifiedNormal);  
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}