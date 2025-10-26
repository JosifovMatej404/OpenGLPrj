#version 330 core
out vec4 FragColor;

uniform vec3 color; // bright sun color

void main()
{
    // Make it very bright so HDR + bloom picks it up
    FragColor = vec4(color, 1.0);
}
