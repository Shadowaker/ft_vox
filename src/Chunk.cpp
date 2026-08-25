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

// Terrain generation constants
static constexpr int   SEA_LEVEL     = 64;
static constexpr int   SNOW_LEVEL    = 140;
static constexpr int   BEACH_LEVEL   = SEA_LEVEL + 3;
static constexpr float CONTINENT_SC  = 0.0008f; // large landmasses
static constexpr float MOUNTAIN_SC   = 0.003f;  // mountain ridges
static constexpr float DETAIL_SC     = 0.015f;  // surface roughness
static constexpr float CAVE_SC       = 0.05f;   // cave tunnels
static constexpr float CAVE_THRESH   = 0.55f;   // cave density
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

				// Cave carving — skip near surface and at bedrock
				if (y > 4 && y < height - 3) {
					float cv = noise.sample3D(wx * CAVE_SC, y * CAVE_SC, wz * CAVE_SC);
					// Use abs(noise) so caves form tubular shapes
					if (std::abs(cv) < (1.0f - CAVE_THRESH))
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

void Chunk::buildMesh() {
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
					if (!transparent(getBlock(x + f.nx, y + f.ny, z + f.nz)))
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
	if (indexCount_ == 0) { dirty_ = false; return; }

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
	dirty_ = false;
}

void Chunk::render() const {
	if (!vao_ || indexCount_ == 0) return;
	glBindVertexArray(vao_);
	glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
	glBindVertexArray(0);
}