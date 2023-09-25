#version 330 core

layout(location = 0) in vec2 position;

uniform mat4 uVP;
uniform mat4 uModel;

void main()
{
    gl_Position = uVP * uModel * vec4(position, 0, 1);
};

