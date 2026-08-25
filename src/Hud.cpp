#include "../include/Hud.hpp"

#include <GL/glew.h>
#include <cctype>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace {

// 5x7 bitmap font, one row per byte, bit4 = leftmost pixel of the 5 columns.
// Only the characters actually needed by the debug HUD (FPS counter + the
// biome names in World::getBiomeAt) are defined; anything else falls back
// to a blank glyph in glyphIndex().
constexpr int NUM_GLYPHS = 26;
constexpr int GLYPH_W    = 5;
constexpr int GLYPH_H    = 7;
constexpr int CELL       = 8; // 5x7 glyph inset by 1px on all sides, for padding

using Glyph = uint8_t[GLYPH_H];

constexpr Glyph FONT[NUM_GLYPHS] = {
	// 0: SPACE
	{0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00000},
	// 1: ':'
	{0b00000, 0b00000, 0b00100, 0b00000, 0b00000, 0b00100, 0b00000},
	// 2-11: '0'-'9'
	{0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}, // 0
	{0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}, // 1
	{0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}, // 2
	{0b11111, 0b00010, 0b00100, 0b00010, 0b00001, 0b10001, 0b01110}, // 3
	{0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}, // 4
	{0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}, // 5
	{0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}, // 6
	{0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}, // 7
	{0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}, // 8
	{0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100}, // 9
	// 12-25: A B C E F H I L N O P S W Y
	{0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}, // A
	{0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110}, // B
	{0b01111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b01111}, // C
	{0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111}, // E
	{0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000}, // F
	{0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001}, // H
	{0b01110, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}, // I
	{0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111}, // L
	{0b10001, 0b11001, 0b10101, 0b10101, 0b10011, 0b10001, 0b10001}, // N
	{0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110}, // O
	{0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000}, // P
	{0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110}, // S
	{0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b11011, 0b10001}, // W
	{0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100}, // Y
};

int glyphIndex(char c) {
	c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	if (c == ' ') return 0;
	if (c == ':') return 1;
	if (c >= '0' && c <= '9') return 2 + (c - '0');
	switch (c) {
		case 'A': return 12; case 'B': return 13; case 'C': return 14;
		case 'E': return 15; case 'F': return 16; case 'H': return 17;
		case 'I': return 18; case 'L': return 19; case 'N': return 20;
		case 'O': return 21; case 'P': return 22; case 'S': return 23;
		case 'W': return 24; case 'Y': return 25;
		default:  return 0; // unsupported char → blank cell
	}
}

} // namespace

unsigned int Hud::buildFontAtlas() {
	const int W = NUM_GLYPHS * CELL;
	const int H = CELL;
	std::vector<unsigned char> data(W * H * 4, 0); // all transparent by default

	for (int g = 0; g < NUM_GLYPHS; g++) {
		for (int row = 0; row < GLYPH_H; row++) {
			for (int col = 0; col < GLYPH_W; col++) {
				bool lit = (FONT[g][row] >> (GLYPH_W - 1 - col)) & 1;
				if (!lit) continue;
				int px = g * CELL + 1 + col; // +1 = padding inset
				int py = 1 + row;
				int i  = (py * W + px) * 4;
				data[i+0] = data[i+1] = data[i+2] = data[i+3] = 255;
			}
		}
	}

	unsigned int tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return tex;
}

Hud::Hud() {
	shader_  = std::make_unique<Shader>("shaders/hud_vertex.glsl", "shaders/hud_fragment.glsl");
	fontTex_ = buildFontAtlas();

	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);

	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	// location 0: position (vec2)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);
	// location 1: texcoord (vec2)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);
}

Hud::~Hud() {
	if (vao_) glDeleteVertexArrays(1, &vao_);
	if (vbo_) glDeleteBuffers(1, &vbo_);
	if (fontTex_) glDeleteTextures(1, &fontTex_);
}

void Hud::renderText(const std::string& text, float x, float y, float scale,
                      int screenW, int screenH) const {
	std::vector<float> verts;
	verts.reserve(text.size() * 6 * 4);

	float pen = x;
	for (char c : text) {
		int   g  = glyphIndex(c);
		float u0 = static_cast<float>(g) / NUM_GLYPHS;
		float u1 = static_cast<float>(g + 1) / NUM_GLYPHS;
		float x0 = pen,               y0 = y;
		float x1 = pen + CELL*scale,  y1 = y + CELL*scale;

		verts.insert(verts.end(), {
			x0, y0, u0, 0.0f,
			x1, y0, u1, 0.0f,
			x1, y1, u1, 1.0f,

			x0, y0, u0, 0.0f,
			x1, y1, u1, 1.0f,
			x0, y1, u0, 1.0f,
		});
		pen += CELL * scale;
	}
	if (verts.empty()) return;

	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screenW),
	                                   static_cast<float>(screenH), 0.0f);

	shader_->use();
	shader_->setMat4("projection", projection);
	// Unit 1, not 0 — unit 0 is where main.cpp permanently binds the world
	// block atlas; reusing it here would clobber that binding every frame.
	shader_->setInt("fontAtlas", 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fontTex_);

	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, static_cast<int>(verts.size() / 4));
	glBindVertexArray(0);
}
