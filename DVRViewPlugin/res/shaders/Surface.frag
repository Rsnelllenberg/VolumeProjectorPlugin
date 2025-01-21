#version 330
out vec4 FragColor;

in vec3 u_color;

void main()
{
    FragColor = vec4(u_color, 1);
}