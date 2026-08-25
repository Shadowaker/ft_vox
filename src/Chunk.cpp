#include "../include/Chunk.hpp"

#include <GL/glew.h>
#include <algorithm>
#include <vector>

Chunk::Chunk(int cx, int cz) : cx_(cx), cz_(cz) {
	blocks_.fill(BlockType::AIR);
}

Chunk::~Chunk() {
	if (vao_) glDeleteVertexArrays(1, &vao_);
	if (vbo_) glDeleteBuffers(1, &vbo_);
	if (ebo_) glDeleteBuffers(1, &ebo_);
	if (caveVao_) glDeleteVertexArrays(1, &caveVao_);
	if (caveVbo_) glDeleteBuffers(1, &caveVbo_);
	if (caveEbo_) glDeleteBuffers(1, &caveEbo_);
	if (waterVao_) glDeleteVertexArrays(1, &waterVao_);
	if (waterVbo_) glDeleteBuffers(1, &waterVbo_);
	if (waterEbo_) glDeleteBuffers(1, &waterEbo_);
}

BlockType Chunk::getBlock(int x, int y, int z) const {
	if (x < 0 || x >= CHUNK_W || y < 0 || y >= CHUNK_H || z < 0 || z >= CHUNK_D)
		return BlockType::AIR;
	return blocks_[y * CHUNK_W * CHUNK_D + z * CHUNK_W + x];
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
	if (x < 0 || x >= CHUNK_W || y < 0 || y >= CHUNK_H || z < 0 || z >= CHUNK_D)
		return;
	blocks_[y * CHUNK_W * CHUNK_D + z * CHUNK_W + x] = type;
	dirty_ = true;
}

bool Chunk::isCaveAir(int x, int y, int z) const {
	if (x < 0 || x >= CHUNK_W || y < 0 || y >= CHUNK_H || z < 0 || z >= CHUNK_D)
		return false; // unknown across chunk borders — treat as not-cave
	if (getBlock(x, y, z) != BlockType::AIR)
		return false;
	return y < surfaceHeight_[z * CHUNK_W + x];
}

BlockType Chunk::getBlockCross(int x, int y, int z, const Chunk* north, const Chunk* south,
                                const Chunk* east, const Chunk* west) const {
	if (y < 0 || y >= CHUNK_H) return BlockType::AIR;
	if (x < 0)         return west  ? west->getBlock(CHUNK_W + x, y, z)  : BlockType::AIR;
	if (x >= CHUNK_W)  return east  ? east->getBlock(x - CHUNK_W, y, z)  : BlockType::AIR;
	if (z < 0)         return north ? north->getBlock(x, y, CHUNK_D + z) : BlockType::AIR;
	if (z >= CHUNK_D)  return south ? south->getBlock(x, y, z - CHUNK_D) : BlockType::AIR;
	return getBlock(x, y, z);
}

bool Chunk::isCaveAirCross(int x, int y, int z, const Chunk* north, const Chunk* south,
                            const Chunk* east, const Chunk* west) const {
	if (y < 0 || y >= CHUNK_H) return false;
	if (x < 0)         return west  && west->isCaveAir(CHUNK_W + x, y, z);
	if (x >= CHUNK_W)  return east  && east->isCaveAir(x - CHUNK_W, y, z);
	if (z < 0)         return north && north->isCaveAir(x, y, CHUNK_D + z);
	if (z >= CHUNK_D)  return south && south->isCaveAir(x, y, z - CHUNK_D);
	return isCaveAir(x, y, z);
}

int Chunk::getSurfaceHeight(int x, int z) const {
	if (x < 0 || x >= CHUNK_W || z < 0 || z >= CHUNK_D)
		return -1;
	return surfaceHeight_[z * CHUNK_W + x];
}

// Terrain generation constants (SEA_LEVEL, SNOW_LEVEL, BEACH_LEVEL are in Chunk.hpp)
static constexpr float CONTINENT_SC  = 0.0008f; // large landmasses
static constexpr float MOUNTAIN_SC   = 0.003f;  // mountain ridges
static constexpr float DETAIL_SC     = 0.015f;  // surface roughness
static constexpr float CAVE_SC       = 0.02f;   // cave cavern noise frequency
// carve air where noise EXCEEDS this cutoff (one-sided, no abs()). |noise| <
// thresh selects a band around the zero-crossing, which is a thin 2D sheet in
// 3D space no matter how it's scaled — measured true diameter ~1.5 blocks,
// matching the "1-2 blocks wide" report. A one-sided cutoff instead carves out
// the noise field's local maxima as actual 3D blobs: measured ~9.5 block
// diameter at 0.4, roomy Minecraft-cavern-like chambers instead of slits.
static constexpr float CAVE_CUTOFF          = 0.4f;   // interior: ~8% air, ~9.5 block diameter
static constexpr float CAVE_ENTRANCE_CUTOFF = 0.5f;    // stricter near surface → rarer breaches (~3% vs ~8%)
static constexpr float WARP_SC       = 0.002f;  // domain-warp scale

void Chunk::generate(const Noise& noise) {
	for (int x = 0; x < CHUNK_W; x++) {
		for (int z = 0; z < CHUNK_D; z++) {
			float wx = static_cast<float>(cx_ * CHUNK_W + x);
			float wz = static_cast<float>(cz_ * CHUNK_D + z);

			// Domain warping: offset sample coords to break grid regularity
			float warpX = noise.fbm2D(wx * WARP_SC,        wz * WARP_SC,        3) * 40.0f;
			float warpZ = noise.fbm2D(wx * WARP_SC + 5.3f, wz * WARP_SC + 9.1f, 3) * 40.0f;
			float sx = wx + warpX;
			float sz = wz + warpZ;

			// Continent: broad elevation bias [-1, 1]
			float continent = noise.fbm2D(sx * CONTINENT_SC, sz * CONTINENT_SC, 5, 0.5f, 2.0f);
			// Mountain ridges: squared to create sharp peaks
			float mountain  = noise.fbm2D(sx * MOUNTAIN_SC,  sz * MOUNTAIN_SC,  5, 0.55f, 2.0f);
			mountain = mountain * mountain; // sharpen peaks
			// Fine surface detail
			float detail    = noise.fbm2D(wx * DETAIL_SC,    wz * DETAIL_SC,    3, 0.45f, 2.1f);

			int height = static_cast<int>(
				SEA_LEVEL
				+ continent * 40.0f
				+ mountain  * 60.0f
				+ detail    *  6.0f
			);
			height = std::clamp(height, 2, CHUNK_H - 2);
			surfaceHeight_[z * CHUNK_W + x] = height;

			bool isSandy = height <= BEACH_LEVEL;
			bool isSnowy = height >= SNOW_LEVEL;

			for (int y = 0; y < CHUNK_H; y++) {
				if (y > height) {
					// Fill water up to sea level
					if (y <= SEA_LEVEL)
						setBlock(x, y, z, BlockType::WATER);
					// else AIR (already default)
					continue;
				}

				// Cave carving — skip at bedrock; stricter near the surface so
				// tunnels only breach as rarer entrances instead of everywhere.
				// Includes y == height so a carved tunnel can punch through
				// the surface skin itself, not just stop just beneath it.
				if (y > 4 && y <= height) {
					float cv = noise.sample3D(wx * CAVE_SC, y * CAVE_SC, wz * CAVE_SC);
					// One-sided: carve the noise field's local maxima as real 3D blobs
					float cutoff = (y >= height - 3) ? CAVE_ENTRANCE_CUTOFF : CAVE_CUTOFF;
					if (cv > cutoff)
						continue; // leave as AIR
				}

				// Block type assignment
				if (y == 0) {
					setBlock(x, y, z, BlockType::STONE); // indestructible base
				} else if (y == height) {
					if      (isSnowy)  setBlock(x, y, z, BlockType::SNOW);
					else if (isSandy)  setBlock(x, y, z, BlockType::SAND);
					else               setBlock(x, y, z, BlockType::GRASS);
				} else if (y >= height - 3) {
					if (isSandy)       setBlock(x, y, z, BlockType::SAND);
					else               setBlock(x, y, z, BlockType::DIRT);
				} else {
					setBlock(x, y, z, BlockType::STONE);
				}
			}
		}
	}
	dirty_ = true;
}

void Chunk::buildMesh(const Chunk* north, const Chunk* south, const Chunk* east, const Chunk* west) {
	// Each face: neighbor offset, faceID, 4 vertices (CCW from outside → correct winding for GL_CULL_FACE)
	struct FaceTemplate {
		int   nx, ny, nz;
		float faceID;
		float vx[4][3];
	};
	static const FaceTemplate faces[6] = {
		{  0, 1, 0, 0.f, {{0,1,1},{1,1,1},{1,1,0},{0,1,0}} }, // TOP
		{  0,-1, 0, 1.f, {{0,0,0},{1,0,0},{1,0,1},{0,0,1}} }, // BOTTOM
		{  0, 0,-1, 2.f, {{1,0,0},{0,0,0},{0,1,0},{1,1,0}} }, // NORTH
		{  0, 0, 1, 3.f, {{0,0,1},{1,0,1},{1,1,1},{0,1,1}} }, // SOUTH
		{  1, 0, 0, 4.f, {{1,0,1},{1,0,0},{1,1,0},{1,1,1}} }, // EAST
		{ -1, 0, 0, 5.f, {{0,0,0},{0,0,1},{0,1,1},{0,1,0}} }, // WEST
	};
	// UV corners: same order for every face
	static const float k_uvs[4][2] = {{0,0},{1,0},{1,1},{0,1}};

	auto transparent = [](BlockType b) {
		return b == BlockType::AIR || b == BlockType::WATER;
	};

	std::vector<float>        verts;
	std::vector<unsigned int> idxs;
	unsigned int              off = 0;

	for (int y = 0; y < CHUNK_H; y++) {
		for (int z = 0; z < CHUNK_D; z++) {
			for (int x = 0; x < CHUNK_W; x++) {
				BlockType blk = getBlock(x, y, z);
				if (transparent(blk)) continue;

				float tid = static_cast<float>(static_cast<int>(blk));

				for (const auto& f : faces) {
					if (!transparent(getBlockCross(x + f.nx, y + f.ny, z + f.nz, north, south, east, west)))
						continue;

					for (int v = 0; v < 4; v++) {
						verts.insert(verts.end(), {
							x + f.vx[v][0],
							y + f.vx[v][1],
							z + f.vx[v][2],
							tid + k_uvs[v][0], // u = typeIndex + corner_u
							k_uvs[v][1],        // v = corner_v
							f.faceID
						});
					}
					idxs.insert(idxs.end(), {off, off+1, off+2, off, off+2, off+3});
					off += 4;
				}
			}
		}
	}

	indexCount_ = static_cast<int>(idxs.size());
	if (indexCount_ > 0) {
		if (!vao_) glGenVertexArrays(1, &vao_);
		if (!vbo_) glGenBuffers(1, &vbo_);
		if (!ebo_) glGenBuffers(1, &ebo_);

		glBindVertexArray(vao_);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_);
		glBufferData(GL_ARRAY_BUFFER,
			verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			idxs.size() * sizeof(unsigned int), idxs.data(), GL_STATIC_DRAW);

		// location 0: position (vec3)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			6 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);
		// location 1: texcoord (vec2)
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
			6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		// location 2: faceID (float)
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE,
			6 * sizeof(float), reinterpret_cast<void*>(5 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
	}

	// Debug mesh: render carved-out underground air as translucent blocks.
	// Uses the atlas tile just past the real block types (see createBlockAtlas
	// in main.cpp), so it needs no BlockType of its own.
	std::vector<float>        caveVerts;
	std::vector<unsigned int> caveIdxs;
	unsigned int              caveOff = 0;
	float                     caveTid = static_cast<float>(static_cast<int>(BlockType::COUNT));

	for (int y = 0; y < CHUNK_H; y++) {
		for (int z = 0; z < CHUNK_D; z++) {
			for (int x = 0; x < CHUNK_W; x++) {
				if (!isCaveAir(x, y, z)) continue;

				for (const auto& f : faces) {
					if (isCaveAirCross(x + f.nx, y + f.ny, z + f.nz, north, south, east, west))
						continue; // hidden between two adjoining void cells

					for (int v = 0; v < 4; v++) {
						caveVerts.insert(caveVerts.end(), {
							x + f.vx[v][0],
							y + f.vx[v][1],
							z + f.vx[v][2],
							caveTid + k_uvs[v][0],
							k_uvs[v][1],
							f.faceID
						});
					}
					caveIdxs.insert(caveIdxs.end(),
						{caveOff, caveOff+1, caveOff+2, caveOff, caveOff+2, caveOff+3});
					caveOff += 4;
				}
			}
		}
	}

	caveIndexCount_ = static_cast<int>(caveIdxs.size());
	if (caveIndexCount_ > 0) {
		if (!caveVao_) glGenVertexArrays(1, &caveVao_);
		if (!caveVbo_) glGenBuffers(1, &caveVbo_);
		if (!caveEbo_) glGenBuffers(1, &caveEbo_);

		glBindVertexArray(caveVao_);

		glBindBuffer(GL_ARRAY_BUFFER, caveVbo_);
		glBufferData(GL_ARRAY_BUFFER,
			caveVerts.size() * sizeof(float), caveVerts.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, caveEbo_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			caveIdxs.size() * sizeof(unsigned int), caveIdxs.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			6 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
			6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE,
			6 * sizeof(float), reinterpret_cast<void*>(5 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
	}

	// Water mesh: WATER is transparent for the solid pass above (so it
	// doesn't hide neighboring terrain), but still needs a visible surface.
	// Only face air — a face against solid ground or another water block
	// would just be redundant/hidden geometry.
	std::vector<float>        waterVerts;
	std::vector<unsigned int> waterIdxs;
	unsigned int              waterOff = 0;
	float                     waterTid = static_cast<float>(static_cast<int>(BlockType::WATER));

	for (int y = 0; y < CHUNK_H; y++) {
		for (int z = 0; z < CHUNK_D; z++) {
			for (int x = 0; x < CHUNK_W; x++) {
				if (getBlock(x, y, z) != BlockType::WATER) continue;

				for (const auto& f : faces) {
					if (getBlockCross(x + f.nx, y + f.ny, z + f.nz, north, south, east, west) != BlockType::AIR)
						continue;

					for (int v = 0; v < 4; v++) {
						waterVerts.insert(waterVerts.end(), {
							x + f.vx[v][0],
							y + f.vx[v][1],
							z + f.vx[v][2],
							waterTid + k_uvs[v][0],
							k_uvs[v][1],
							f.faceID
						});
					}
					waterIdxs.insert(waterIdxs.end(),
						{waterOff, waterOff+1, waterOff+2, waterOff, waterOff+2, waterOff+3});
					waterOff += 4;
				}
			}
		}
	}

	waterIndexCount_ = static_cast<int>(waterIdxs.size());
	if (waterIndexCount_ > 0) {
		if (!waterVao_) glGenVertexArrays(1, &waterVao_);
		if (!waterVbo_) glGenBuffers(1, &waterVbo_);
		if (!waterEbo_) glGenBuffers(1, &waterEbo_);

		glBindVertexArray(waterVao_);

		glBindBuffer(GL_ARRAY_BUFFER, waterVbo_);
		glBufferData(GL_ARRAY_BUFFER,
			waterVerts.size() * sizeof(float), waterVerts.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEbo_);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			waterIdxs.size() * sizeof(unsigned int), waterIdxs.data(), GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
			6 * sizeof(float), reinterpret_cast<void*>(0));
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
			6 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE,
			6 * sizeof(float), reinterpret_cast<void*>(5 * sizeof(float)));
		glEnableVertexAttribArray(2);

		glBindVertexArray(0);
	}

	dirty_ = false;
}

void Chunk::renderWater() const {
	if (!waterVao_ || waterIndexCount_ == 0) return;
	glBindVertexArray(waterVao_);
	glDrawElements(GL_TRIANGLES, waterIndexCount_, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}

void Chunk::render(bool showCaveDebug) const {
	if (showCaveDebug) {
		if (!caveVao_ || caveIndexCount_ == 0) return;
		glBindVertexArray(caveVao_);
		glDrawElements(GL_TRIANGLES, caveIndexCount_, GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);
		return;
	}
	if (!vao_ || indexCount_ == 0) return;
	glBindVertexArray(vao_);
	glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}