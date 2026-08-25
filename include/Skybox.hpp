#pragma once

#include "Shader.hpp"
#include <glm/glm.hpp>
#include <memory>

// A giant cube rendered as a plain color gradient (no cubemap texture — no
// image files/libraries are used anywhere else in the project yet either).
// Its depth is forced to the far plane and its view matrix has camera
// translation stripped, so it always fills the background and never appears
// to move as the camera does — the standard skybox technique.
class Skybox {
public:
	Skybox();
	~Skybox();

	// Draw first, before any other geometry — see main.cpp for the GL state
	// (depth func/write, culling) this needs around it.
	void render(const glm::mat4& view, const glm::mat4& projection) const;

private:
	std::unique_ptr<Shader> shader_;
	unsigned int            vao_ = 0;
	unsigned int            vbo_ = 0;
};
