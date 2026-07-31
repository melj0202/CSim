#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// Faded display colors (RGB). Domain state stays on the CPU as lifeCanvas.
uniform sampler2D uDisplayTexture;

void main()
{
	FragColor = texture(uDisplayTexture, TexCoord);
}
