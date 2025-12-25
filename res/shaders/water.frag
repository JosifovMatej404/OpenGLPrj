#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoords;
in vec4 ReflectedClipPos;
in mat3 TBN;

uniform vec3 viewPos;
uniform vec3 islandPos;
uniform float time;

uniform sampler2D reflectionTex;
uniform sampler2D shadowMap;
uniform samplerCube skyCubemap;
uniform sampler2D normalMap;

uniform mat4 lightSpaceMatrix;
uniform float normalStrength;



float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.x < 0.0 || projCoords.x > 1.0 ||
        projCoords.y < 0.0 || projCoords.y > 1.0)
        return 0.0;

    float bias = 0.005;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            shadow += projCoords.z - bias >
                      texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r
                      ? 1.0 : 0.0;

    return shadow / 9.0;
}

void main()
{
    vec3 viewDir = normalize(viewPos - FragPos);


    // ===== ISLAND INFLUENCE (LIMITED) =====
    float dist = length(FragPos.xz - islandPos.xz);
    float islandInfluence = 1.0 - smoothstep(0.0, 30.0, dist);

    // ===== ORIGINAL NORMAL ANIMATION =====
    vec2 uv = FragPos.xz * 0.05; // world-space tiling (infinite-safe)

    vec2 distortion =
        vec2(sin(time + dist), cos(time + dist)) * 0.01 * islandInfluence;

    vec3 normalTex =
        texture(normalMap, uv + distortion).xyz * 2.0 - 1.0;

    normalTex.xy *= normalStrength;

    vec3 waterNormal = normalize(TBN * normalTex);

    // ===== FRESNEL =====
    float ndotv = max(dot(waterNormal, viewDir), 0.0);

    // Fresnel with base reflectivity (F0)
    float F0 = 0.02;                 // water ≈ 2%
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - ndotv, 3.0);

    // artistic dampening
    fresnel *= 0.65;


    // ===== REFLECTION =====
    float distToCam = distance(FragPos, viewPos);

    // planar reflections only near camera
    float planarFade = clamp(1.0 - distToCam / 150.0, 0.0, 1.0);

    vec2 ndc = ReflectedClipPos.xy / ReflectedClipPos.w;
    vec2 reflUV = ndc * 0.5 + 0.5 + waterNormal.xz * 0.05;

    reflUV = clamp(reflUV, 0.001, 0.999);

    vec3 planarReflection = texture(reflectionTex, reflUV).rgb;
    vec3 skyReflection = texture(
        skyCubemap,
        reflect(-viewDir, waterNormal)
    ).rgb;



    // fade planar → sky
    vec3 reflection = mix(skyReflection, planarReflection, planarFade);


    // ===== SHADOW =====
    float shadow =
    ShadowCalculation(lightSpaceMatrix * vec4(FragPos, 1.0));

    shadow *= islandInfluence; // you already computed this


    // ===== FINAL COLOR =====
    vec3 waterColor = vec3(0.0, 0.3, 0.5);
    vec3 color = mix(waterColor, reflection, fresnel);
    color *= 1.0 - shadow * 0.5;

    // Tune these numbers based on your far plane
    float horizonFade = smoothstep(1000.0, 5500.0, distToCam);

    // Blend water into sky reflection near horizon
    color = mix(color, skyReflection, horizonFade);

    FragColor = vec4(color, 0.7);
}
