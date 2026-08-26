#pragma once


#include <cmath>
#include <array>
#include <cstdint>

/*
 * Sources:
 * https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/perlin-noise-part-2/improved-perlin-noise.html
 * https://en.wikipedia.org/wiki/Smoothstep
 */


// Improved Perlin Noise implementation
class Noise {
	public:
		explicit Noise(uint64_t seed);

		float sample2D(float x, float y) const;
		float sample3D(float x, float y, float z) const;

		// Fractional Brownian Motion, sum of octaves
		float fbm2D(float x, float y,
		            int octaves, float persistence = 0.5f, float lacunarity = 2.0f) const;
		float fbm3D(float x, float y, float z,
		            int octaves, float persistence = 0.5f, float lacunarity = 2.0f) const;

	private:
		std::array<int, 512> _perm;

		static float fade(float t);
		static float lerp(float a, float b, float t);
		static float grad2(int hash, float x, float y);
		static float grad3(int hash, float x, float y, float z);
	};