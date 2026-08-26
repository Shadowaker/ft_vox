#pragma once


#include "Noise.hpp"
#include <cmath>
#include <array>
#include <cstdint>
#include <glm/glm.hpp>

class Noise;

enum class BlockType : uint8_t {
	AIR = 0,
	GRASS,
	DIRT,
	STONE,
	SAND,
	SNOW,
	WATER,
	FOREST_GRASS,
	COUNT
};

enum class Biome : uint8_t {
	OCEAN,
	BEACH,
	SNOWY_PEAKS,
	DESERT,
	PLAINS,
	FOREST,
	MOUNTAINS,
};

const char* biomeName(Biome b);

constexpr int CHUNK_W = 16;
constexpr int CHUNK_H = 256;
constexpr int CHUNK_D = 16;

// Height bands used for both block placement (Chunk::generate) and biome
// classification (Chunk::classifyBiome), kept in one place so they can't
// drift apart.
constexpr int SEA_LEVEL   = 64;
constexpr int SNOW_LEVEL  = 140;
constexpr int BEACH_LEVEL = SEA_LEVEL + 3;

class Chunk {
public:
	Chunk(int cx, int cz);
	~Chunk();

	void generate(const Noise& noise);
	// Neighbor chunks (already-loaded, may be null) let mesh building see
	// past this chunk's own edge instead of assuming AIR there, without
	// them, chunk borders show phantom faces (visible as seams in water).
	void buildMesh(const Chunk* north = nullptr, const Chunk* south = nullptr,
	               const Chunk* east = nullptr,  const Chunk* west = nullptr);
	void markDirty() { dirty_ = true; }
	// showCaveDebug: draw the translucent underground-air visualization mesh
	// instead of the normal solid terrain mesh (see World::render).
	void render(bool showCaveDebug = false) const;
	// Translucent water surface, separate mesh/draw call so it can be
	// rendered with blending after opaque terrain (see World::renderWater).
	void renderWater() const;

	BlockType getBlock(int x, int y, int z) const;
	void      setBlock(int x, int y, int z, BlockType type);

	bool       isDirty() const { return dirty_; }
	glm::ivec2 getPos() const { return {cx_, cz_}; }

	// Surface height (terrain top, pre-cave-carving) at a local column.
	// Returns -1 if (x, z) is outside the chunk.
	int getSurfaceHeight(int x, int z) const;

	Biome getBiome(int x, int z) const;

private:
	static Biome classifyBiome(int height, float temperature, float humidity);

	// True for AIR blocks enclosed underground (below the column's surface
	// height), i.e. actual carved cave space, not open sky above terrain.
	bool isCaveAir(int x, int y, int z) const;

	// Cross-chunk versions: fall back to the given neighbor when (x, y, z)
	// steps outside this chunk (a face offset only ever crosses one axis at
	// a time, so only one of the four neighbors is ever consulted). A null
	// neighbor (not loaded) falls back to AIR/not-cave, same as before.
	BlockType getBlockCross(int x, int y, int z, const Chunk* north, const Chunk* south,
	                         const Chunk* east, const Chunk* west) const;
	bool isCaveAirCross(int x, int y, int z, const Chunk* north, const Chunk* south,
	                     const Chunk* east, const Chunk* west) const;

	int  cx_, cz_;
	bool dirty_ = true;

	std::array<BlockType, CHUNK_W * CHUNK_H * CHUNK_D> blocks_{};
	std::array<int, CHUNK_W * CHUNK_D>                 surfaceHeight_{};
	std::array<Biome, CHUNK_W * CHUNK_D>               biome_{};

	unsigned int vao_ = 0;
	unsigned int vbo_ = 0;
	unsigned int ebo_ = 0;
	int          indexCount_ = 0;
	size_t       vaoBytes_ = 0;

	// Debug mesh: cave-air voxels rendered as translucent blocks
	unsigned int caveVao_ = 0;
	unsigned int caveVbo_ = 0;
	unsigned int caveEbo_ = 0;
	int          caveIndexCount_ = 0;
	size_t       caveBytes_ = 0;

	// Water mesh: WATER blocks are transparent for the solid-mesh pass (so
	// they don't hide neighboring terrain faces) but need their own visible
	// surface, drawn as a separate translucent pass.
	unsigned int waterVao_ = 0;
	unsigned int waterVbo_ = 0;
	unsigned int waterEbo_ = 0;
	int          waterIndexCount_ = 0;
	size_t       waterBytes_ = 0;
};