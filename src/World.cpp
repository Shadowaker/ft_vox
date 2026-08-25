#include "World.hpp"
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

World::World(uint64_t seed) : seed_(seed), noise_(seed) {}

void World::update(const glm::vec3& playerPos) {
	int playerCX = static_cast<int>(std::floor(playerPos.x / CHUNK_W));
	int playerCZ = static_cast<int>(std::floor(playerPos.z / CHUNK_D));

	for (int dx = -RENDER_DISTANCE; dx <= RENDER_DISTANCE; dx++) {
		for (int dz = -RENDER_DISTANCE; dz <= RENDER_DISTANCE; dz++) {
			ChunkKey key{playerCX + dx, playerCZ + dz};
			if (!chunks_.contains(key))
				loadChunk(key.x, key.z);
		}
	}

	unloadDistantChunks(playerCX, playerCZ);

	for (auto& [key, chunk] : chunks_) {
		if (chunk->isDirty())
			chunk->buildMesh();
	}
}

void World::render(const Shader& shader, const glm::mat4& vp, bool showCaveDebug) const {
	frustum_.update(vp);

	for (const auto& [key, chunk] : chunks_) {
		// Build world-space AABB for this chunk and cull against the frustum
		AABB box;
		box.min = glm::vec3(key.x * CHUNK_W,          0,        key.z * CHUNK_D);
		box.max = glm::vec3(key.x * CHUNK_W + CHUNK_W, CHUNK_H, key.z * CHUNK_D + CHUNK_D);
		if (!frustum_.intersects(box))
			continue;

		glm::mat4 model = glm::translate(glm::mat4(1.0f),
			glm::vec3(key.x * CHUNK_W, 0.0f, key.z * CHUNK_D));
		shader.setMat4("model", model);
		chunk->render(showCaveDebug);
	}
}

void World::loadChunk(int cx, int cz) {
	auto chunk = std::make_unique<Chunk>(cx, cz);
	chunk->generate(noise_);
	chunks_[ChunkKey{cx, cz}] = std::move(chunk);
}

void World::unloadDistantChunks(int playerCX, int playerCZ) {
	for (auto it = chunks_.begin(); it != chunks_.end();) {
		int dx = std::abs(it->first.x - playerCX);
		int dz = std::abs(it->first.z - playerCZ);
		if (dx > RENDER_DISTANCE + 2 || dz > RENDER_DISTANCE + 2)
			it = chunks_.erase(it);
		else
			++it;
	}
}