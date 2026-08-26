#include "../include/Skybox.hpp"
#include "../include/GpuMemory.hpp"

#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>

namespace {

// 36 vertices, position-only, a unit cube meant to be viewed from the
// inside. Winding doesn't matter, Skybox::render is drawn with face
// culling disabled.
constexpr float kVertices[] = {
	-1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,
};

} // namespace

Skybox::Skybox() {
	shader_ = std::make_unique<Shader>("shaders/skybox_vertex.glsl", "shaders/skybox_fragment.glsl");

	glGenVertexArrays(1, &vao_);
	glGenBuffers(1, &vbo_);

	glBindVertexArray(vao_);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_);
	glBufferData(GL_ARRAY_BUFFER, sizeof(kVertices), kVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), reinterpret_cast<void*>(0));
	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	GpuMemory::adjust(static_cast<long long>(sizeof(kVertices)));
}

Skybox::~Skybox() {
	if (vao_) glDeleteVertexArrays(1, &vao_);
	if (vbo_) glDeleteBuffers(1, &vbo_);
	GpuMemory::adjust(-static_cast<long long>(sizeof(kVertices)));
}

void Skybox::render(const glm::mat4& view, const glm::mat4& projection) const {
	// Strip translation so the skybox never moves relative to the camera,
	// only rotation affects it, keeping it infinitely distant.
	glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

	shader_->use();
	shader_->setMat4("view", viewNoTranslation);
	shader_->setMat4("projection", projection);
	shader_->setVec3("zenithColor",  glm::vec3(0.25f, 0.55f, 0.90f));
	shader_->setVec3("horizonColor", glm::vec3(0.80f, 0.90f, 1.00f));
	shader_->setVec3("nadirColor",   glm::vec3(0.30f, 0.35f, 0.40f));

	glBindVertexArray(vao_);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}
