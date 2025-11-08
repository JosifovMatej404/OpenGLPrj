#version 330 core
out vec4 FragColor;

in vec3 TexCoords;
uniform samplerCube skybox;
uniform float exposure = 0.5; // tweak for brightness

void main()
{
    vec3 color = texture(skybox, TexCoords).rgb;
    color = pow(color, vec3(2.2)); // gamma correction
    color *= exposure;             // scale brightness
    FragColor = vec4(color, 1.0);
}
