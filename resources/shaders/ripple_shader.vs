#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec3 FragPos;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float currentFrame;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));

    vec2 direction;
    direction.x = FragPos.x - 0.76;
    direction.y = FragPos.z + 0.87;

    float distance = length(direction);

    TexCoords = aTexCoords;
    TexCoords.x -= 0.5;
    TexCoords.y -= 0.5;

    float curFr = currentFrame;
    while(curFr > 0.8) curFr-=0.8;

    TexCoords.x /= curFr*8;
    TexCoords.y /= curFr*8;

    TexCoords.x += 0.5;
    TexCoords.y += 0.5;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

