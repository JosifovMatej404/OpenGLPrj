#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 ReflectedClipPos;
in mat3 TBN;

uniform vec3 lightDir;
uniform vec3 viewPos;

uniform sampler2D reflectionTex;
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;

uniform float seaLevel;

uniform samplerCube skyCubemap;

uniform sampler2D normalMap;
uniform float normalStrength;
uniform float time;
uniform vec3 islandPos;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // discard if outside shadow map
    if(projCoords.x < 0.0 || projCoords.x > 1.0 ||
       projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;

    float bias = 0.005;
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for(int x=-1;x<=1;x++)
        for(int y=-1;y<=1;y++)
            shadow += projCoords.z - bias > texture(shadowMap, projCoords.xy + vec2(x,y)*texelSize).r ? 1.0 : 0.0;

    shadow /= 9.0;

    if(projCoords.z > 1.0) shadow = 0.0;

    return shadow;
}

void main()
{
    vec3 viewDir = normalize(FragPos - viewPos);

    // --- Radial + Tangential offsets for normal animation ---
    vec2 fragXZ = FragPos.xz - islandPos.xz;
    float dist = length(fragXZ);
    vec2 radial = normalize(-fragXZ);
    vec2 tangent = vec2(-radial.y, radial.x);

    float falloff = smoothstep(0.0, 5.0, dist);

    vec2 uvOffset = vec2(0.0);
    uvOffset += radial * 0.02 * sin(time * 0.8 + dist * 0.5);
    uvOffset += tangent * 0.015 * cos(time * 1.2 + dist * 0.7);
    uvOffset += radial * 0.01 * cos(time * 1.7 + dist * 0.3);
    uvOffset += tangent * 0.008 * sin(time * 2.3 + dist * 0.9);

    uvOffset += vec2(0.01 * sin(dist * 1.2 + time * 0.7),
                     0.01 * cos(dist * 1.5 + time * 0.9));

    vec2 animatedUV = TexCoords * 10.0 + uvOffset * falloff;

    // --- Sample normal ---
    vec3 normalTex = texture(normalMap, animatedUV).xyz;
    normalTex = normalTex * 2.0 - 1.0;
    normalTex.xy *= normalStrength;
    vec3 waterNormal = normalize(TBN * normalTex);

    // --- Fresnel ---
    float fresnel = pow(1.0 - max(dot(waterNormal, -viewDir), 0.0), 3.0);

    // --- Reflections ---
    vec2 ndc = ReflectedClipPos.xy / ReflectedClipPos.w;
    vec2 reflUV = ndc * 0.5 + 0.5 + waterNormal.xz * 0.05;
    reflUV = clamp(reflUV, 0.001, 0.999);
    vec3 planarReflection = texture(reflectionTex, reflUV).rgb;
    vec3 skyReflection = texture(skyCubemap, reflect(viewDir, waterNormal)).rgb;
    vec3 reflection = mix(planarReflection, skyReflection, fresnel);

    // --- Shadow at flat sea level ---
    // --- Shadow at water surface position (after vertex displacement) ---
    float shadow = ShadowCalculation(lightSpaceMatrix * vec4(FragPos, 1.0));


    // --- Final color ---
    vec3 waterColor = vec3(0.0, 0.3, 0.5);
    vec3 color = mix(waterColor, reflection, fresnel);
    color *= 1.0 - shadow * 0.5;

    FragColor = vec4(color, 0.7);
}
