#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// R8 cell-state texture (0..255 encoded as normalized RED).
uniform sampler2D uCellTexture;
// 256×1 RGB palette; sample at ((state + 0.5) / 256, 0.5).
uniform sampler2D uPalette;

void main()
{
	float r = texture(uCellTexture, TexCoord).r;
	// Map normalized RED back to state byte, then to palette texel center.
	float u = (r * 255.0 + 0.5) / 256.0;
	vec3 color = texture(uPalette, vec2(u, 0.5)).rgb;
	FragColor = vec4(color, 1.0);
}
