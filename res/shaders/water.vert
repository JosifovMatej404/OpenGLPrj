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
    vec3 pos = aPos;

    // --- Superposition of multiple waves ---
    float wave1 = sin(pos.x * 0.5 + pos.z * 0.4 + time * 0.8) * 0.03;
    float wave2 = cos(pos.x * 0.45 - pos.z * 0.55 + time * 0.9) * 0.025;
    float wave3 = sin((pos.x + pos.z) * 0.48 + time * 1.0) * 0.02;
    float wave4 = cos((pos.x - pos.z) * 0.35 + time * 1.2) * 0.015;
    float wave5 = sin((pos.x * 0.6 + pos.z * 0.5) + time * 0.7) * 0.01;

    float totalWave = wave1 + wave2 + wave3 + wave4 + wave5;

    pos.y += totalWave;

    // Clamp height
    pos.y = clamp(pos.y, minHeight, maxHeight);

    vec4 worldPos = model * vec4(pos, 1.0);
    FragPos = worldPos.xyz;

    // Transform normal
    Normal = normalize(mat3(transpose(inverse(model))) * aNormal);

    // TBN for normal mapping
    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    TBN = mat3(T, B, N);

    TexCoords = aTexCoords;

    // Reflection clip position
    ReflectedClipPos = reflectionVP * worldPos;

    gl_Position = projection * view * worldPos;
}
