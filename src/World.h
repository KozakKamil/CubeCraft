#pragma once
#include "Chunk.h"
#include "FastNoiseLite.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>

struct IVec2Hash {
    size_t operator()(const glm::ivec2& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};
struct RaycastHit {
    bool hit = false;
    glm::ivec3 blockPos;
    glm::ivec3 prevPos;
    BlockType blockType = BlockType::Air;
};

class World {
public:
    World(int seed = 1337);

    void generateInitial(int radius);
    void draw(class Shader& shader) const;

    Chunk* getChunk(int cx, int cz);
    BlockType getBlockWorld(int wx, int wy, int wz);
    void setBlockWorld(int wx, int wy, int wz, BlockType type);

    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDist = 5.0f);

private:
    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, IVec2Hash> m_chunks;
    FastNoiseLite m_noise;
    int m_seed;

    void generateChunkTerrain(Chunk& chunk);
    void chunkCoordsFromWorld(int wx, int wz, int& cx, int& cz, int& lx, int& lz);
};