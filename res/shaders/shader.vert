#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float clipHeight;
uniform int clipAbove;
out float clipDist;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * worldPos;
    FragPos = vec3(model * vec4(aPos,1.0));

    clipDist = clipAbove == 1
        ? worldPos.y - clipHeight
        : clipHeight - worldPos.y;
}
