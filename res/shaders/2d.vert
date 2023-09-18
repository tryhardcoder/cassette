#version 330 core

layout(location = 0) in vec2 position;

uniform mat4 uVP;
uniform mat4 uModel;

uniform vec2 uSrcStart;
uniform vec2 uSrcEnd;

out vec2 vUv;

vec2 cornerTable[4] = vec2[](
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 1),
    vec2(1, 0)
    );

void main()
{
    vUv = cornerTable[gl_VertexID];
    vUv *= uSrcEnd - uSrcStart;
    vUv += uSrcStart;

    gl_Position = uVP * uModel * vec4(position, 0, 1);
};

