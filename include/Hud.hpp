#pragma once

#include "Shader.hpp"
#include <memory>
#include <string>

// Minimal on-screen text renderer for debug overlays (FPS counter, biome
// name, ...). No font files are loaded — the glyphs are a small hardcoded
// 5x7 bitmap font baked into a generated texture atlas, the same way
// createBlockAtlas() builds the world texture programmatically in main.cpp.
class Hud {
public:
	Hud();
	~Hud();

	// Draws one line of text, left-aligned, top-left corner at (x, y) in
	// pixel space (origin top-left, y down). Only supports the characters
	// covered by the built-in font (A-Z digits space : — see glyphIndex());
	// anything else renders as a blank cell.
	void renderText(const std::string& text, float x, float y, float scale,
	                 int screenW, int screenH) const;

private:
	static unsigned int buildFontAtlas();

	std::unique_ptr<Shader> shader_;
	unsigned int            fontTex_ = 0;
	unsigned int            vao_ = 0;
	unsigned int            vbo_ = 0;
};
