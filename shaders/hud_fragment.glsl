#version 410 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D fontAtlas;

void main() {
	vec4 color = texture(fontAtlas, TexCoord);
	if (color.a < 0.01)
		discard;
	FragColor = color;
}
