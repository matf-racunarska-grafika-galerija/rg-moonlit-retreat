#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float currentFrame;

void main()
{
    TexCoords.x = aTexCoords.x;

    TexCoords.y = aTexCoords.y - 3*currentFrame;
    TexCoords.x = aTexCoords.x + sin(currentFrame)/3;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}