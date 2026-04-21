#pragma once
#include "Chunk.h"
#include "FastNoiseLite.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>

// Hash dla glm::ivec2 (klucz mapy chunkow)
struct IVec2Hash {
    size_t operator()(const glm::ivec2& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};

class World {
public:
    World(int seed = 2137);

    void generateInitial(int radius);
    void draw(class Shader& shader) const;
    Chunk* getChunk(int cx, int cz);
    BlockType getBlockWorld(int wx, int wy, int wz);

private:
    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, IVec2Hash> m_chunks;
    FastNoiseLite m_noise;
    int m_seed;

    void generateChunkTerrain(Chunk& chunk);
};