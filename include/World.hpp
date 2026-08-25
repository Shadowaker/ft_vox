#pragma once

#include "Chunk.hpp"
#include "Frustum.hpp"
#include "Noise.hpp"
#include "Shader.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

struct ChunkKey {
	int  x, z;
	bool operator==(const ChunkKey& o) const { return x == o.x && z == o.z; }
};

struct ChunkKeyHash {
	size_t operator()(const ChunkKey& k) const {
		return std::hash<int>()(k.x) ^ (std::hash<int>()(k.z) << 16);
	}
};

class World {
public:
	static constexpr int      RENDER_DISTANCE = 10; // chunks (160 cubes min)
	static constexpr uint64_t DEFAULT_SEED    = 42;

	World(uint64_t seed = DEFAULT_SEED);

	void update(const glm::vec3& playerPos);
	// vp = projection * view, used to cull chunks outside the frustum.
	// showCaveDebug: render the translucent underground-air visualization.
	void render(const Shader& shader, const glm::mat4& vp, bool showCaveDebug = false) const;
	// Translucent water surfaces, drawn as a separate pass after opaque
	// terrain (see main.cpp for the blend/depth-write state around this).
	void renderWater(const Shader& shader, const glm::mat4& vp) const;

	// Name of the biome at this world position's column, derived from the
	// same height bands used during terrain generation. "UNKNOWN" if the
	// containing chunk hasn't been generated/loaded yet.
	std::string getBiomeAt(const glm::vec3& pos) const;

private:
	void loadChunk(int cx, int cz);
	void unloadDistantChunks(int playerCX, int playerCZ);

	uint64_t        seed_;
	Noise           noise_;
	mutable Frustum frustum_;
	std::unordered_map<ChunkKey, std::unique_ptr<Chunk>, ChunkKeyHash> chunks_;
};