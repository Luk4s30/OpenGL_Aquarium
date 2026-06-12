#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse1;
uniform bool hasTexture;   
uniform vec3 matColor;     

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

void main() {
    vec4 baseColor;
    if (hasTexture) {
        baseColor = texture(texture_diffuse1, TexCoords);
        if (baseColor.a < 0.1) discard;
    } else {
        baseColor = vec4(matColor, 1.0);
    }

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(spotLight.position - FragPos);

    float diff = dot(norm, lightDir) * 0.5 + 0.5;

    float theta = dot(lightDir, normalize(-spotLight.direction));
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);

    float distance = length(spotLight.position - FragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));

    vec3 diffuse = spotLight.color * diff * intensity * attenuation;
    vec3 ambient = vec3(0.25) * baseColor.rgb;

    vec3 result = ambient + (diffuse * baseColor.rgb);

    FragColor = vec4(result, baseColor.a);
    FragColor += calculateChestGlow(norm, FragPos);
}
