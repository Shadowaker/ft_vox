#version 410 core

in vec2  TexCoord;
in float FaceID;

out vec4 FragColor;

uniform sampler2D textureAtlas;

const float ATLAS_W = 7.0;

// Directional lighting per face
float faceLight(float id) {
	if (id < 0.5) return 1.00; // TOP    — full light
	if (id < 1.5) return 0.40; // BOTTOM — darkest
	if (id < 2.5) return 0.75; // NORTH
	if (id < 3.5) return 0.80; // SOUTH
	               return 0.65; // EAST / WEST
}

void main() {
	vec2 atlasUV = vec2(TexCoord.x / ATLAS_W, TexCoord.y);
	vec4 color   = texture(textureAtlas, atlasUV);

	if (color.a < 0.01)
		discard;

	FragColor = vec4(color.rgb * faceLight(FaceID), color.a);
}