#version 330 core

in vec4 ourColor;
in vec2 ourUv;

out vec4 FragColor;

uniform sampler2D uTexture;

void main()
{
    FragColor = texture(uTexture, ourUv) * ourColor;
}
