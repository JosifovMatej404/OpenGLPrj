#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out vec4 ReflectedClipPos;
out mat3 TBN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 reflectionVP;

uniform float time;
uniform float minHeight;
uniform float maxHeight;

void main()
{
    // --- WORLD POSITION FIRST ---
    vec4 worldPos = model * vec4(aPos, 1.0);
    vec3 worldXZ = worldPos.xyz;

    // --- SAME WAVES, BUT WORLD-SPACE ---
    float wave1 = sin(worldXZ.x * 0.5 + worldXZ.z * 0.4 + time * 0.8) * 0.03;
    float wave2 = cos(worldXZ.x * 0.45 - worldXZ.z * 0.55 + time * 0.9) * 0.025;
    float wave3 = sin((worldXZ.x + worldXZ.z) * 0.48 + time * 1.0) * 0.02;
    float wave4 = cos((worldXZ.x - worldXZ.z) * 0.35 + time * 1.2) * 0.015;
    float wave5 = sin((worldXZ.x * 0.6 + worldXZ.z * 0.5) + time * 0.7) * 0.01;

    float totalWave = wave1 + wave2 + wave3 + wave4 + wave5;

    worldPos.y += totalWave;
    worldPos.y = clamp(worldPos.y, minHeight, maxHeight);

    FragPos = worldPos.xyz;

    // --- NORMALS ---
    Normal = normalize(mat3(transpose(inverse(model))) * aNormal);

    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    TBN = mat3(T, B, N);

    TexCoords = aTexCoords;

    ReflectedClipPos = reflectionVP * worldPos;
    gl_Position = projection * view * worldPos;
}
