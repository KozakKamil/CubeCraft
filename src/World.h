#pragma once
#include "Chunk.h"
#include "FastNoiseLite.h"
#include "CaveGenerator.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

struct IVec2Hash {
    size_t operator()(const glm::ivec2& v) const {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};
struct IVec2Eq {
    bool operator()(const glm::ivec2& a, const glm::ivec2& b) const {
        return a.x == b.x && a.y == b.y;
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
    ~World();

    void update(const glm::vec3& cameraPos, int renderDistance);
    void draw(class Shader& shader) const;

    Chunk* getChunk(int cx, int cz);
    BlockType getBlockWorld(int wx, int wy, int wz);
    void setBlockWorld(int wx, int wy, int wz, BlockType type);

    RaycastHit raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDist = 5.0f);

    size_t chunkCount() const { return m_chunks.size(); }

private:
    std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>, IVec2Hash> m_chunks;
    FastNoiseLite m_noiseContinental;
    FastNoiseLite m_noiseHilliness;
    FastNoiseLite m_noiseDetail;
    FastNoiseLite m_noiseCaves;     
    FastNoiseLite m_noiseCaves2;    
    FastNoiseLite m_noiseOres;
    int m_seed;
    CaveGenerator m_caveGen;

    std::vector<std::thread> m_workers;
    std::mutex m_queueMutex;
    std::condition_variable m_cv;
    std::queue<glm::ivec2> m_requestQueue;
    std::queue<std::unique_ptr<Chunk>> m_readyQueue;
    std::unordered_set<glm::ivec2, IVec2Hash, IVec2Eq> m_inFlight;
    std::atomic<bool> m_running{ true };
	std::unordered_set<glm::ivec2, IVec2Hash, IVec2Eq> m_dirtyMeshes;

    static constexpr int WORKER_THREADS = 2;
    static constexpr int MAX_MESH_UPLOADS_PER_FRAME = 1;
    static constexpr int MAX_DIRTY_REBUILDS_PER_FRAME = 1;
    static constexpr int MAX_CHUNKS_UNLOADED_PER_FRAME = 1;

    void workerLoop();

    void generateChunkTerrain(Chunk& chunk);
    void generateChunkTrees(Chunk& chunk);
    int  sampleHeight(int wx, int wz);
    BlockType sampleTopBlock(int height);
    bool shouldSpawnTree(int wx, int wz);
    bool shouldSpawnTreeCandidate(int wx, int wz);
    void placeTreeAt(Chunk& chunk, int wx, int wz, int baseY);

    void chunkCoordsFromWorld(int wx, int wz, int& cx, int& cz, int& lx, int& lz);
};