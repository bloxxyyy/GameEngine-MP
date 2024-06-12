#version 330 core

out vec4 FragColor;

in vec2 TexCoord;
in vec3 ourColor;

uniform sampler2D ourTexture;
uniform sampler2D texture2;

void main()
{
    FragColor = mix(texture(ourTexture, TexCoord), texture(texture2, TexCoord), 0.5) * vec4(ourColor, 1.0);
}