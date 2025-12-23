#version 330 core

in vec3 FragPos;
in vec3 Normal;
in float clipDist;   // <-- comes from vertex shader

out vec4 FragColor;

uniform vec3 viewPos;
uniform vec3 lightDir;
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

uniform int clipAbove;

uniform float seaLevel;

// Shadow calculation unchanged
float ShadowCalculation(vec4 fragPosLightSpace, vec3 norm)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0)
        return 0.0;

    float shadow = 0.0;
    float bias = max(0.002 * (1.0 - dot(norm, -lightDir)), 0.01);
    float texelSize = 1.0 / textureSize(shadowMap, 0).x;

    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }

    return shadow / 9.0;
}

void main()
{
    // ---------- CLIPPING ----------
    if (clipAbove != -1 && clipDist < 0.0)
        discard;


    // ---------- HEIGHT FADE ----------
    float fadeStart = seaLevel + 0.011;
    float fadeEnd   = seaLevel + 0.05;

    if (FragPos.y <= fadeStart)
    discard;

    float alpha = smoothstep(fadeStart, fadeEnd, FragPos.y);

    // ---------- TERRAIN COLOR ----------
    vec3 colorYellow = vec3(0.875, 0.859, 0.710);
    vec3 colorBlue   = vec3(0.0, 0.35, 0.5);
    vec3 colorGreen  = vec3(0.263, 0.706, 0.424);
    vec3 colorGray   = vec3(0.447, 0.329, 0.157);
    vec3 colorWhite  = vec3(0.898, 0.851, 0.761);

    float h = FragPos.y;
    vec3 baseColor = mix(colorBlue, colorYellow, smoothstep(0.0, 0.05, h));
    baseColor = mix(baseColor, colorGreen, smoothstep(0.05, 0.1, h));
    baseColor = mix(baseColor, colorGray, smoothstep(0.2, 0.7, h));

    // ---------- LIGHTING ----------
    vec3 norm = normalize(Normal);
    vec3 lightDirection = normalize(-lightDir);

    vec3 ambient = 0.1 * baseColor;
    float diff = max(dot(norm, lightDirection), 0.0);
    vec3 diffuse = diff * baseColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirection, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = 0.05 * spec * vec3(1.0);

    vec4 fragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    float shadow = ShadowCalculation(fragPosLightSpace, norm);

    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);

    FragColor = vec4(result, alpha);
}
