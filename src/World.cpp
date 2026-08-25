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
		if (!chunk->isDirty())
			continue;
		// North/south/east/west match the Chunk face-template convention
		// (north = -z, south = +z, east = +x, west = -x).
		auto neighbor = [this](int cx, int cz) -> const Chunk* {
			auto it = chunks_.find(ChunkKey{cx, cz});
			return it != chunks_.end() ? it->second.get() : nullptr;
		};
		chunk->buildMesh(neighbor(key.x, key.z - 1), neighbor(key.x, key.z + 1),
		                 neighbor(key.x + 1, key.z), neighbor(key.x - 1, key.z));
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

void World::renderWater(const Shader& shader, const glm::mat4& vp) const {
	frustum_.update(vp);

	for (const auto& [key, chunk] : chunks_) {
		AABB box;
		box.min = glm::vec3(key.x * CHUNK_W,          0,        key.z * CHUNK_D);
		box.max = glm::vec3(key.x * CHUNK_W + CHUNK_W, CHUNK_H, key.z * CHUNK_D + CHUNK_D);
		if (!frustum_.intersects(box))
			continue;

		glm::mat4 model = glm::translate(glm::mat4(1.0f),
			glm::vec3(key.x * CHUNK_W, 0.0f, key.z * CHUNK_D));
		shader.setMat4("model", model);
		chunk->renderWater();
	}
}

std::string World::getBiomeAt(const glm::vec3& pos) const {
	int wx = static_cast<int>(std::floor(pos.x));
	int wz = static_cast<int>(std::floor(pos.z));
	int cx = static_cast<int>(std::floor(pos.x / CHUNK_W));
	int cz = static_cast<int>(std::floor(pos.z / CHUNK_D));

	auto it = chunks_.find(ChunkKey{cx, cz});
	if (it == chunks_.end())
		return "UNKNOWN";

	int lx = wx - cx * CHUNK_W;
	int lz = wz - cz * CHUNK_D;
	int height = it->second->getSurfaceHeight(lx, lz);
	if (height < 0)
		return "UNKNOWN";

	if (height <= SEA_LEVEL)   return "OCEAN";
	if (height >= SNOW_LEVEL)  return "SNOWY PEAKS";
	if (height <= BEACH_LEVEL) return "BEACH";
	return "PLAINS";
}

void World::loadChunk(int cx, int cz) {
	auto chunk = std::make_unique<Chunk>(cx, cz);
	chunk->generate(noise_);
	chunks_[ChunkKey{cx, cz}] = std::move(chunk);

	// Any already-meshed neighbor built its shared edge assuming AIR here
	// (this chunk didn't exist yet) — force it to rebuild against the real
	// data now that it does, or the seam persists even after this loads.
	static constexpr int offsets[4][2] = {{0,-1}, {0,1}, {1,0}, {-1,0}};
	for (auto& [dx, dz] : offsets) {
		auto it = chunks_.find(ChunkKey{cx + dx, cz + dz});
		if (it != chunks_.end())
			it->second->markDirty();
	}
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