#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// Nasze 3 tekstury
uniform sampler2D baseColorMap;
uniform sampler2D normalMap;
uniform sampler2D aoMap;

uniform vec3 viewPos;

layout (std140) uniform LightBlock {
    vec3 position;
    float cutOff;
    vec3 direction;
    float outerCutOff;
    vec3 color;
    float constant;
    float linear;
    float quadratic;
} spotLight;

layout (std140) uniform ChestLightBlock {
    vec3  position;
    float intensity;
    vec3  color;
    float pad;
} chestLight;

vec3 calculateChestGlow(vec3 normal, vec3 fragPos) {
    vec3  dir         = normalize(chestLight.position - fragPos);
    float dist        = length(chestLight.position - fragPos);
    float attenuation = 1.0 / (1.0 + 0.8 * dist + 3.0 * dist * dist);
    float diff        = max(dot(normal, dir), 0.0);
    return chestLight.color * diff * attenuation * chestLight.intensity * 3.0;
}

// Funkcja magicznie wyliczająca wypukłości bez potrzeby wektorów Tangent!
vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, TexCoords).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(FragPos);
    vec3 Q2  = dFdy(FragPos);
    vec2 st1 = dFdx(TexCoords);
    vec2 st2 = dFdy(TexCoords);

    vec3 N   = normalize(Normal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

void main() {
    // Pobieramy dane z tekstur
    vec3 color = texture(baseColorMap, TexCoords).rgb;
    vec3 normal = getNormalFromMap();
    // Ambient Occlusion (zaciemnienie w rogach) z tekstury
    float ao = texture(aoMap, TexCoords).r; 
    
    vec3 lightDir = normalize(spotLight.position - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    // 1. Światło otoczenia - tutaj aplikujemy AO, by przyciemnić zakamarki skrzyni!
    vec3 ambient = vec3(0.3) * color * ao;
    
    // 2. Diffuse (światło rozproszone) na podstawie wypukłości (normal)
    float diff = max(dot(normal, lightDir), 0.0);
    
    // 3. Specular (odblask) - drewno słabo błyszczy, więc dajemy niski połysk (16.0) i siłę (0.2)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 16.0);
    vec3 specular = spotLight.color * spec * 0.2;
    
    // Stożek latarki
    float theta = dot(lightDir, normalize(-spotLight.direction)); 
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    
    // Tłumienie odległością
    float distance = length(spotLight.position - FragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));
    
    vec3 diffuse = spotLight.color * diff * color;
    
    // Sumujemy światło
    vec3 result = ambient + (diffuse + specular) * intensity * attenuation;
    
    vec3 chestGlow = calculateChestGlow(normal, FragPos);
    vec3 finalResult = ambient + (diffuse + specular) * intensity * attenuation + chestGlow;
    FragColor = vec4(finalResult, 1.0);
    //FragColor = vec4(result, 1.0);
}