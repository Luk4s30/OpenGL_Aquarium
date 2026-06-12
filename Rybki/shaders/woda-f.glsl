#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in float isTopFace;

uniform vec4 baseColor;
uniform vec3 viewPos; 
uniform samplerCube skybox; 
uniform float time; 

uniform sampler2D screenTexture; 
uniform vec2 resolution;

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

vec3 calculateWaterSpotLight(vec3 normal, vec3 fragPos, vec3 viewDir, bool isTopSurface) {
    vec3 lightDir = normalize(spotLight.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 specular = vec3(0.0);
    
    float theta = dot(lightDir, normalize(-spotLight.direction)); 
    float epsilon = spotLight.cutOff - spotLight.outerCutOff;
    float intensity = clamp((theta - spotLight.outerCutOff) / epsilon, 0.0, 1.0);
    
    if (isTopSurface) {
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 128.0);
        specular = spotLight.color * spec * intensity;
    }
    
    float distance = length(spotLight.position - fragPos);
    float attenuation = 1.0 / (spotLight.constant + spotLight.linear * distance + spotLight.quadratic * (distance * distance));
    
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
    bool isTopSurface = (isTopFace > 0.5);
    
    if (isTopSurface) {
        float wave1 = sin((FragPos.x + FragPos.z) * 8.0 + time * 3.0);
        float wave2 = cos((FragPos.x - FragPos.z) * 6.0 - time * 2.0);
        float wave3 = sin((FragPos.x * 0.5 - FragPos.z * 1.2) * 12.0 + time * 4.0);
        
        norm.x += (wave1 + wave3) * 0.015;
        norm.z += (wave2 - wave3) * 0.015;
        norm = normalize(norm);
    }

    vec3 viewDir = normalize(viewPos - FragPos);
    float depthGradient = smoothstep(-1.0, 0.8, FragPos.y);
    vec3 waterTint = baseColor.rgb * (0.6 + 0.4 * depthGradient);
    vec3 lighting = calculateWaterSpotLight(norm, FragPos, viewDir, isTopSurface);
    
    vec3 ambient = vec3(0.2) * waterTint;
    vec3 finalColor = ambient + (lighting * waterTint);
    
    float finalAlpha = isTopSurface ? 1.0 : 0.08; 

    if (isTopSurface) {
        vec2 screenUV = gl_FragCoord.xy / resolution;
        
        // Zniekształcenie UV
        vec2 distortion = norm.xz * 0.003;
        vec3 refraction = texture(screenTexture, screenUV + distortion).rgb;

        finalColor = mix(refraction, waterTint, 0.25);
        finalColor += lighting; 

        // Odbicia nieba
        vec3 I = normalize(FragPos - viewPos);
        vec3 R = reflect(I, norm);
        vec3 reflection = texture(skybox, R).rgb;
        
        float NdotV = abs(dot(norm, viewDir));
        float fresnel = pow(1.0 - NdotV, 3.0);
        
        finalColor = mix(finalColor, reflection, fresnel * 0.6);
        finalColor += vec3(fresnel * 0.15); 
    }
    
    FragColor = vec4(finalColor, finalAlpha);
    FragColor += calculateChestGlow(norm, FragPos);
}