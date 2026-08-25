#include "../include/Noise.hpp"

#include <algorithm>
#include <numeric>
#include <random>

/* Same seed, same output */
Noise::Noise(const unsigned long int seed) {
	std::iota(_perm.begin(), _perm.begin() + 256, 0);
	std::mt19937_64 rng(seed);
	std::shuffle(_perm.begin(), _perm.begin() + 256, rng);

	for (int i = 0; i < 256; i++)
		_perm[256 + i] = _perm[i];
}

float Noise::fade(float t) {
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float Noise::lerp(float a, float b, float t) {
	return a + t * (b - a);
}

float Noise::grad2(int hash, float x, float y) {
	switch (hash & 3) {
		case 0: return x + y;
		case 1: return -x + y;
		case 2: return x - y;
		case 3: return -x - y;
		default: return 0.0f;
	}
}

float Noise::grad3(int hash, float x, float y, float z) {
	int h = hash & 15;
	float u = h < 8 ? x : y;
	float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

float Noise::sample2D(float x, float y) const {
	int X = static_cast<int>(std::floor(x)) & 255;
	int Y = static_cast<int>(std::floor(y)) & 255;
	x -= std::floor(x);
	y -= std::floor(y);
	float u = fade(x), v = fade(y);
	int a = _perm[X] + Y, b = _perm[X + 1] + Y;
	return lerp(
		lerp(grad2(_perm[a], x, y),
		     grad2(_perm[b], x - 1, y), u),
		lerp(grad2(_perm[a + 1], x, y - 1),
		     grad2(_perm[b + 1], x - 1, y - 1), u),
		v
	);
}

float Noise::sample3D(float x, float y, float z) const {
	int X = static_cast<int>(std::floor(x)) & 255;
	int Y = static_cast<int>(std::floor(y)) & 255;
	int Z = static_cast<int>(std::floor(z)) & 255;
	x -= std::floor(x);
	y -= std::floor(y);
	z -= std::floor(z);
	float u = fade(x), v = fade(y), w = fade(z);
	int a = _perm[X] + Y, aa = _perm[a] + Z, ab = _perm[a + 1] + Z;
	int b = _perm[X + 1] + Y, ba = _perm[b] + Z, bb = _perm[b + 1] + Z;
	return lerp(
		lerp(lerp(grad3(_perm[aa], x, y, z),
		          grad3(_perm[ba], x - 1, y, z), u),
		     lerp(grad3(_perm[ab], x, y - 1, z),
		          grad3(_perm[bb], x - 1, y - 1, z), u), v),
		lerp(lerp(grad3(_perm[aa + 1], x, y, z - 1),
		          grad3(_perm[ba + 1], x - 1, y, z - 1), u),
		     lerp(grad3(_perm[ab + 1], x, y - 1, z - 1),
		          grad3(_perm[bb + 1], x - 1, y - 1, z - 1), u), v),
		w
	);
}

float Noise::fbm2D(float x, float y, int octaves, float persistence, float lacunarity) const {
	float value = 0.0f, amp = 1.0f, freq = 1.0f, maxVal = 0.0f;
	for (int i = 0; i < octaves; i++) {
		value += sample2D(x * freq, y * freq) * amp;
		maxVal += amp;
		amp *= persistence;
		freq *= lacunarity;
	}
	return value / maxVal;
}

float Noise::fbm3D(float x, float y, float z, int octaves, float persistence, float lacunarity) const {
	float value = 0.0f, amp = 1.0f, freq = 1.0f, maxVal = 0.0f;
	for (int i = 0; i < octaves; i++) {
		value += sample3D(x * freq, y * freq, z * freq) * amp;
		maxVal += amp;
		amp *= persistence;
		freq *= lacunarity;
	}
	return value / maxVal;
}
