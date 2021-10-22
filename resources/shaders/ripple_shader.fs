#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoords;

uniform sampler2D texture1;

void main()
{
    float dist = distance(FragPos, vec3(-0.76, 1.01, 0.87));
    if(dist > 1.3) discard;


    vec4 temp = texture(texture1, TexCoords);


    temp /= 3;
    temp.w = 1.0;
    FragColor = temp;
}

// -0.76, 1.01, 0.87 center