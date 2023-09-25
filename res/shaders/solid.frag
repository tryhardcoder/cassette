#version 330 core

out vec4 color;
uniform vec4 uColor;

void main()
{
    color = uColor;
    if (color.a <= 0.01) { discard; }
};
