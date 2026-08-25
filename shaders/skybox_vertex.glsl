#version 410 core

layout (location = 0) in vec3 aPos;

out vec3 Dir; // local cube position doubles as a direction from the camera

uniform mat4 view;       // rotation only — translation stripped by Skybox::render
uniform mat4 projection;

void main() {
	Dir = aPos;
	vec4 pos = projection * view * vec4(aPos, 1.0);
	// Force depth to the far plane (z/w = 1.0 after the divide) regardless
	// of the cube's actual size, so it always renders behind real geometry
	// without needing to match the camera's far clip distance.
	gl_Position = pos.xyww;
}
