#version 330 core
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 lightDir;
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

// Shadow calculation unchanged
float ShadowCalculation(vec4 fragPosLightSpace, vec3 norm)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0) return 0.0;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;

    float bias = max(0.002 * (1.0 - dot(norm, normalize(-lightDir))), 0.0005);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }

    shadow /= 9.0;
    shadow *= 0.6; 
    return shadow;
}

void main()
{
    // --- Smooth height-based color ---
    vec3 colorBlue = vec3(0.0, 0.0, 1.0);
    vec3 colorGreen = vec3(0.0, 1.0, 0.0);
    vec3 colorGray = vec3(0.5, 0.5, 0.5);
    vec3 colorWhite = vec3(1.0, 1.0, 1.0);

    // Smoothstep ranges (adjust these for your terrain)
    float h = FragPos.y;
    vec3 baseColor = colorBlue;

    baseColor = mix(colorBlue, colorGreen, smoothstep(0.0, 0.05, h));
    baseColor = mix(baseColor, colorGray, smoothstep(0.05, 0.4, h));
    baseColor = mix(baseColor, colorWhite, smoothstep(1.0, 2.0, h));

    // --- Normal & light calculations ---
    vec3 norm = normalize(Normal);
    vec3 perturbedNormal = normalize(norm + 0.2 * vec3(
        fract(sin(dot(FragPos.xy, vec2(12.9898,78.233))) * 43758.5453),
        fract(sin(dot(FragPos.yz, vec2(12.9898,78.233))) * 43758.5453),
        fract(sin(dot(FragPos.xz, vec2(12.9898,78.233))) * 43758.5453)
    ));

    vec3 lightDirection = normalize(-lightDir);

    float ambientStrength = 0.4;
    vec3 ambient = ambientStrength * baseColor;

    float diff = max(dot(perturbedNormal, lightDirection), 0.0);
    vec3 diffuse = diff * baseColor;

    float specularStrength = 0.05;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, perturbedNormal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);
    vec3 specular = specularStrength * spec * vec3(1.0);

    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    float shadow = ShadowCalculation(fragPosLightSpace, perturbedNormal);

    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);
    FragColor = vec4(result, 1.0);
}
