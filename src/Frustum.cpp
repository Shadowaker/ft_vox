#include "Frustum.hpp"
#include <glm/glm.hpp>

// Gribb-Hartmann plane extraction.
// For a column-major matrix M, row i = (M[0][i], M[1][i], M[2][i], M[3][i]).
// A clip-space point p is inside if -w <= x,y,z <= w, which translates to
// six half-space inequalities on the row vectors:
//   Left:   row3 + row0 >= 0
//   Right:  row3 - row0 >= 0
//   Bottom: row3 + row1 >= 0
//   Top:    row3 - row1 >= 0
//   Near:   row3 + row2 >= 0
//   Far:    row3 - row2 >= 0
void Frustum::update(const glm::mat4& m) {
	glm::vec4 rows[4];
	for (int i = 0; i < 4; i++)
		rows[i] = glm::vec4(m[0][i], m[1][i], m[2][i], m[3][i]);

	planes_[0] = rows[3] + rows[0]; // Left
	planes_[1] = rows[3] - rows[0]; // Right
	planes_[2] = rows[3] + rows[1]; // Bottom
	planes_[3] = rows[3] - rows[1]; // Top
	planes_[4] = rows[3] + rows[2]; // Near
	planes_[5] = rows[3] - rows[2]; // Far

	// Normalise so the w component gives the true signed distance
	for (auto& p : planes_) {
		float len = glm::length(glm::vec3(p));
		if (len > 0.0f) p /= len;
	}
}

// For each frustum plane, find the AABB corner most aligned with the plane
// normal (the "positive vertex"). If even that corner is outside the plane,
// the entire box is outside — cull it.
bool Frustum::intersects(const AABB& box) const {
	for (const auto& p : planes_) {
		glm::vec3 pv(
			p.x >= 0.0f ? box.max.x : box.min.x,
			p.y >= 0.0f ? box.max.y : box.min.y,
			p.z >= 0.0f ? box.max.z : box.min.z
		);
		if (p.x * pv.x + p.y * pv.y + p.z * pv.z + p.w < 0.0f)
			return false;
	}
	return true;
}