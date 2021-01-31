#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube skybox;
uniform bool celShading;

void main()
{
    vec4 result = texture(skybox, TexCoords);
    if(celShading){
        if(result.x > 0.9) result.x = 0.9;
        else if(result.x > 0.8 && result.x < 0.9) result.x = 0.8;
        else if(result.x > 0.7 && result.x < 0.8) result.x = 0.7;
        else if(result.x > 0.6 && result.x < 0.7) result.x = 0.6;
        else if(result.x > 0.5 && result.x < 0.6) result.x = 0.5;
        else if(result.x > 0.4 && result.x < 0.5) result.x = 0.4;
        else if(result.x > 0.3 && result.x < 0.4) result.x = 0.3;
        else if(result.x > 0.2 && result.x < 0.3) result.x = 0.2;
        else if(result.x > 0.1 && result.x < 0.2) result.x = 0.1;
        else if(result.x < 0.1) result.x = 0.0;

        if(result.y > 0.9) result.y = 0.9;
        else if(result.y > 0.8 && result.y < 0.9) result.y = 0.8;
        else if(result.y > 0.7 && result.y < 0.8) result.y = 0.7;
        else if(result.y > 0.6 && result.y < 0.7) result.y = 0.6;
        else if(result.y > 0.5 && result.y < 0.6) result.y = 0.5;
        else if(result.y > 0.4 && result.y < 0.5) result.y = 0.4;
        else if(result.y > 0.3 && result.y < 0.4) result.y = 0.3;
        else if(result.y > 0.2 && result.y < 0.3) result.y = 0.2;
        else if(result.y > 0.1 && result.y < 0.2) result.y = 0.1;
        else if(result.y < 0.1) result.y = 0.0;

        if(result.z > 0.9) result.z = 0.9;
        else if(result.z > 0.8 && result.z < 0.9) result.z = 0.8;
        else if(result.z > 0.7 && result.z < 0.8) result.z = 0.7;
        else if(result.z > 0.6 && result.z < 0.7) result.z = 0.6;
        else if(result.z > 0.5 && result.z < 0.6) result.z = 0.5;
        else if(result.z > 0.4 && result.z < 0.5) result.z = 0.4;
        else if(result.z > 0.3 && result.z < 0.4) result.z = 0.3;
        else if(result.z > 0.2 && result.z < 0.3) result.z = 0.2;
        else if(result.z > 0.1 && result.z < 0.2) result.z = 0.1;
        else if(result.z < 0.1) result.z = 0.0;
    }

    FragColor = result;

}