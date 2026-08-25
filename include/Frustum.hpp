#pragma once

#include <array>
#include <glm/glm.hpp>

// Axis-aligned bounding box in world space
struct AABB {
	glm::vec3 min;
	glm::vec3 max;
};

// View frustum extracted from a combined view-projection matrix.
// Used to skip rendering chunks that are entirely outside the camera's FOV.
class Frustum {
public:
	// Call every frame with (projection * view) before rendering.
	void update(const glm::mat4& vp);

	// Returns true if the AABB is fully or partially inside the frustum.
	bool intersects(const AABB& box) const;

private:
	// 6 planes in the form (a, b, c, d): a*x + b*y + c*z + d >= 0 means inside.
	std::array<glm::vec4, 6> planes_;
};