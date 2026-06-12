#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

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

uniform vec3 viewPos;
uniform sampler2D diffuseTexture;
uniform sampler2D normalMap;

void main()
{
    vec3 normalFromMap = texture(normalMap, TexCoords).rgb;
    normalFromMap = normalFromMap * 2.0 - 1.0;   
    
    vec3 norm = normalize(TBN * normalFromMap); 
    vec3 texColor = texture(diffuseTexture, TexCoords).rgb;
    vec3 ambient = 0.3 * texColor;
    vec3 lightDir = normalize(spotLight.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = spotLight.color * diff * texColor;
    
    float theta = dot(lightDir, normalize(-spotLight.direction)); 
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    
    vec3 chestGlow = calculateChestGlow(norm, FragPos);
    diffuse *= intensity;
    
    vec3 result = ambient + diffuse + chestGlow;
    FragColor = vec4(result, 1.0);
}