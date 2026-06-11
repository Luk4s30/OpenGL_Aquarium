#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent; // Odbieramy nasz nowy wektor

out vec3 FragPos;
out vec2 TexCoords;
out mat3 TBN; // Wysyłamy macierz do fragment shadera

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    TexCoords = aTexCoords;
    
    // Obliczanie wektorów T, B, N do macierzy TBN
    vec3 T = normalize(vec3(model * vec4(aTangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(aNormal, 0.0)));
    
    // Upewniamy się, że T i N są idealnie prostopadłe (tzw. ortogonalizacja Grama-Schmidta)
    T = normalize(T - dot(T, N) * N);
    
    // Obliczamy wektor Bitangent za pomocą iloczynu wektorowego
    vec3 B = cross(N, T);
    
    // Tworzymy macierz TBN
    TBN = mat3(T, B, N);

    gl_Position = projection * view * vec4(FragPos, 1.0);
}