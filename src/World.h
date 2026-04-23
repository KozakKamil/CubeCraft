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
    World(int seed = 25978752);
    size_t chunkCount() const { return m_chunks.size(); }
    void update(const glm::vec3& cameraPos, int renderDistance);

    void draw(class Shader& shader) const;

    Chunk* getChunk(int cx, int cz);
    BlockType getBlockWorld(int wx, int wy, int wz);
    void setBlockWorld(int wx, int wy, int wz, BlockType type);

    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDist = 5.0f);

private:
    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, IVec2Hash> m_chunks;
    FastNoiseLite m_noiseContinental;
    FastNoiseLite m_noiseHilliness;
    FastNoiseLite m_noiseDetail;

    int m_seed;

    static constexpr int MAX_CHUNKS_LOADED_PER_FRAME = 2;
    static constexpr int MAX_CHUNKS_UNLOADED_PER_FRAME = 4;

    void generateChunkTerrain(Chunk& chunk);
	void generateChunkTrees(Chunk& chunk);
	int sampleHeight(int wx, int wz);
	BlockType sampleTopBlock(int height);
	bool shouldSpawnTree(int wx, int wz);
    bool shouldSpawnTreeCandidate(int wx, int wz);
	void placeTreeAt(Chunk& chunk, int wx, int wz, int baseY);
    void chunkCoordsFromWorld(int wx, int wz, int& cx, int& cz, int& lx, int& lz);
};