#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 uv;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vPosition;
out vec2 vUV;
out vec3 vWorldPos;

void main()
{
    vPosition = position;
    vUV = uv;
    vWorldPos = (uModel * vec4(position, 1.0)).xyz;
    gl_Position = uProjection * uView * uModel * vec4(position, 1.0);
}
