#pragma once
#include "Block.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

class World;

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

    uint8_t getBlockLight(int x, int y, int z) const {
        if (x < 0 || x >= SIZE_X || y < 0 || y >= SIZE_Y || z < 0 || z >= SIZE_Z) return 0;
        return m_light[x][y][z] & 0x0F;
    }
    uint8_t getSkyLight(int x, int y, int z) const {
        if (x < 0 || x >= SIZE_X || y < 0 || y >= SIZE_Y || z < 0 || z >= SIZE_Z) return 15;
        return (m_light[x][y][z] >> 4) & 0x0F;
    }
    void setBlockLight(int x, int y, int z, uint8_t v) {
        if (x < 0 || x >= SIZE_X || y < 0 || y >= SIZE_Y || z < 0 || z >= SIZE_Z) return;
        m_light[x][y][z] = (m_light[x][y][z] & 0xF0) | (v & 0x0F);
    }
    void setSkyLight(int x, int y, int z, uint8_t v) {
        if (x < 0 || x >= SIZE_X || y < 0 || y >= SIZE_Y || z < 0 || z >= SIZE_Z) return;
        m_light[x][y][z] = (m_light[x][y][z] & 0x0F) | ((v & 0x0F) << 4);
    }

    // Lacznie - max ze sky/block. Tego uzywa mesh.
    uint8_t getCombinedLight(int x, int y, int z) const {
        uint8_t s = (m_light[x][y][z] >> 4) & 0x0F;
        uint8_t b = m_light[x][y][z] & 0x0F;
        return s > b ? s : b;
    }

    void setWorld(World* w) { m_world = w; }

    glm::ivec3 chunkPos() const { return m_pos; }
    glm::vec3 worldPos() const {
        return glm::vec3(m_pos.x * SIZE_X, m_pos.y * SIZE_Y, m_pos.z * SIZE_Z);
    }

private:
    glm::ivec3 m_pos;
    BlockType m_bloks[SIZE_X][SIZE_Y][SIZE_Z]{};
    uint8_t m_light[SIZE_X][SIZE_Y][SIZE_Z]{};

    GLuint m_VAO = 0, m_VBO = 0;
    GLsizei m_vertexCount = 0;
    bool m_glInitialized = false;

    bool inBounds(int x, int y, int z) const;
    BlockType getBlockForCulling(int x, int y, int z) const;
    void initGL();

    World* m_world = nullptr;
};