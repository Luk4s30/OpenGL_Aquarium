#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec4 baseColor;
uniform vec3 viewPos;
uniform samplerCube skybox;

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

vec3 calculateSpotLight(vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(spotLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    float theta = dot(lightDir, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 256.0);
    vec3 specular = spotLight.color * spec * intensity;

    float distance = length(spotLight.position - fragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance
                              + spotLight.quadratic * (distance * distance));

    return (spotLight.color * diff * intensity + specular) * attenuation;
}

vec3 calculateChestGlow(vec3 normal, vec3 fragPos) {
    vec3  dir         = normalize(chestLight.position - fragPos);
    float dist        = length(chestLight.position - fragPos);
    float attenuation = 1.0 / (1.0 + 0.8 * dist + 3.0 * dist * dist);
    float diff        = max(dot(normal, dir), 0.0);
    return chestLight.color * diff * attenuation * chestLight.intensity * 3.0;
}

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // ↓ wywołanie bez przekazywania struktury
    vec3 lighting = calculateSpotLight(norm, FragPos, viewDir);
    vec3 ambient = vec3(0.05) * baseColor.rgb;
    vec3 finalColor = ambient + lighting;

    vec3 I = normalize(FragPos - viewPos);
    vec3 R = reflect(I, norm);
    vec3 reflection = texture(skybox, R).rgb;

    float fresnel = pow(1.0 - max(dot(norm, viewDir), 0.0), 2.5);
    finalColor = mix(finalColor, reflection, fresnel * 0.7);
    finalColor = mix(finalColor, vec3(0.8, 0.9, 1.0), fresnel * 0.3);

    float finalAlpha = 0.05 + (fresnel * 0.25);
    finalAlpha = clamp(finalAlpha, 0.0, 1.0);

    FragColor = vec4(finalColor, finalAlpha);
    FragColor += calculateChestGlow(norm, FragPos);
}