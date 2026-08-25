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
	COUNT
};

constexpr int CHUNK_W = 16;
constexpr int CHUNK_H = 256;
constexpr int CHUNK_D = 16;

class Chunk {
public:
	Chunk(int cx, int cz);
	~Chunk();

	void generate(const Noise& noise);
	void buildMesh();
	void render() const;

	BlockType getBlock(int x, int y, int z) const;
	void      setBlock(int x, int y, int z, BlockType type);

	bool       isDirty() const { return dirty_; }
	glm::ivec2 getPos() const { return {cx_, cz_}; }

private:
	int  cx_, cz_;
	bool dirty_ = true;

	std::array<BlockType, CHUNK_W * CHUNK_H * CHUNK_D> blocks_{};

	unsigned int vao_ = 0;
	unsigned int vbo_ = 0;
	unsigned int ebo_ = 0;
	int          indexCount_ = 0;
};