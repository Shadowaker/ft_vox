#version 410 core

in vec3 Dir;
out vec4 FragColor;

uniform vec3 zenithColor;
uniform vec3 horizonColor;
uniform vec3 nadirColor;

void main() {
	vec3 d = normalize(Dir);
	// Two-stop gradient around the horizon (d.y == 0): brightens toward the
	// zenith looking up, dims toward the nadir looking down.
	vec3 color = (d.y >= 0.0)
		? mix(horizonColor, zenithColor, smoothstep(0.0, 1.0, d.y))
		: mix(horizonColor, nadirColor,  smoothstep(0.0, 1.0, -d.y));
	FragColor = vec4(color, 1.0);
}
