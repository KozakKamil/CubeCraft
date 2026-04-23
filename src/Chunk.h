#pragma once
#include "Block.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class Chunk {
public:
	static constexpr int SIZE_X = 16;
	static constexpr int SIZE_Y = 128;
	static constexpr int SIZE_Z = 16;

	Chunk(glm::ivec3 chunkPos);
	~Chunk();

	Chunk(const Chunk&) = delete;
	Chunk& operator=(const Chunk&) = delete;

	void generateTerrain();

	void buildMesh();

	void draw() const;

	BlockType getBlock(int x, int y, int z) const;
	void setBlock(int x, int y, int z, BlockType t);

	glm::ivec3 chunkPos() const { return m_pos; }
	glm::vec3 worldPos() const {
		return glm::vec3(m_pos.x * SIZE_X, m_pos.y * SIZE_Y, m_pos.z * SIZE_Z);
	}

private:
	glm::ivec3 m_pos;
	BlockType m_bloks[SIZE_X][SIZE_Y][SIZE_Z]{};

	GLuint m_VAO = 0, m_VBO = 0;
	GLsizei m_vertexCount = 0;
	bool inBounds(int x, int y, int z) const;
};