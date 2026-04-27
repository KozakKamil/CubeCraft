#include "World.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <cstdint>
#include "LightEngine.h"

static uint32_t hash21(int x, int z, uint32_t seed) {
    uint32_t h = seed;
    h ^= (uint32_t)x * 374761393u;
	h ^= (uint32_t)z * 668265263u;
	h = (h ^ (h >> 13)) * 1274126177u;
	return h ^ (h >> 16);
}

static float rand01(int x, int z, uint32_t seed) {
    return (hash21(x, z, seed) & 0xFFFFFF) / float(0x1000000);
}

World::World(int seed) : m_seed(seed), m_caveGen(seed), m_oreGen(seed) {
    m_noiseContinental.SetSeed(seed);
    m_noiseContinental.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseContinental.SetFrequency(0.005f);
    m_noiseContinental.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseContinental.SetFractalOctaves(3);

    m_noiseHilliness.SetSeed(seed + 1);
    m_noiseHilliness.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseHilliness.SetFrequency(0.003f);
    m_noiseHilliness.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseHilliness.SetFractalOctaves(2);

    m_noiseDetail.SetSeed(seed + 2);
    m_noiseDetail.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseDetail.SetFrequency(0.025f);
    m_noiseDetail.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseDetail.SetFractalOctaves(5);
    m_noiseDetail.SetFractalLacunarity(2.0f);
    m_noiseDetail.SetFractalGain(0.55f);

    // === GORY: ridged noise dla ostrych grzbietow ===
    m_noiseMountains.SetSeed(seed + 5);
    m_noiseMountains.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseMountains.SetFrequency(0.0035f);
    m_noiseMountains.SetFractalType(FastNoiseLite::FractalType_Ridged);
    m_noiseMountains.SetFractalOctaves(4);
    m_noiseMountains.SetFractalLacunarity(2.2f);
    m_noiseMountains.SetFractalGain(0.55f);

    // === EROZJA: gdzie sa gory, gdzie niziny ===
    m_noiseErosion.SetSeed(seed + 6);
    m_noiseErosion.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseErosion.SetFrequency(0.0015f);
    m_noiseErosion.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseErosion.SetFractalOctaves(2);

    // === 3D KLIFY ===
    m_noiseCliff3D.SetSeed(seed + 7);
    m_noiseCliff3D.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_noiseCliff3D.SetFrequency(0.055f);
    m_noiseCliff3D.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseCliff3D.SetFractalOctaves(3);

    m_noiseCaves.SetSeed(seed + 3);
    m_noiseCaves.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_noiseCaves.SetFrequency(0.018f);
    m_noiseCaves.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseCaves.SetFractalOctaves(2);

    m_noiseCaves2.SetSeed(seed + 33);
    m_noiseCaves2.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_noiseCaves2.SetFrequency(0.018f);
    m_noiseCaves2.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseCaves2.SetFractalOctaves(2);

    m_noiseOres.SetSeed(seed + 4);
    m_noiseOres.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_noiseOres.SetFrequency(0.12f);

    for (int i = 0; i < WORKER_THREADS; i++) {
        m_workers.emplace_back([this] { workerLoop(); });
    }
}

World::~World() {
    {
        std::lock_guard<std::mutex> lk(m_queueMutex);
        m_running = false;
    }
    m_cv.notify_all();
    for (auto& t : m_workers) {
        if (t.joinable()) t.join();
    }
}

Chunk* World::getChunk(int cx, int cz) {
    auto it = m_chunks.find({ cx, cz });
    return (it != m_chunks.end()) ? it->second.get() : nullptr;
}

void World::chunkCoordsFromWorld(int wx, int wz, int& cx, int& cz, int& lx, int& lz) {
    cx = (wx >= 0) ? wx / Chunk::SIZE_X : (wx - Chunk::SIZE_X + 1) / Chunk::SIZE_X;
    cz = (wz >= 0) ? wz / Chunk::SIZE_Z : (wz - Chunk::SIZE_Z + 1) / Chunk::SIZE_Z;
    lx = wx - cx * Chunk::SIZE_X;
    lz = wz - cz * Chunk::SIZE_Z;
}

BlockType World::getBlockWorld(int wx, int wy, int wz) {
    if (wy < 0 || wy >= Chunk::SIZE_Y) return BlockType::Air;
    int cx, cz, lx, lz;
    chunkCoordsFromWorld(wx, wz, cx, cz, lx, lz);
    Chunk* c = getChunk(cx, cz);
    if (!c) return BlockType::Air;
    return c->getBlock(lx, wy, lz);
}

void World::setBlockWorld(int wx, int wy, int wz, BlockType type) {
    if (wy < 0 || wy >= Chunk::SIZE_Y) return;
    int cx, cz, lx, lz;
    chunkCoordsFromWorld(wx, wz, cx, cz, lx, lz);
    Chunk* c = getChunk(cx, cz);
    if (!c) return;

    c->setBlock(lx, wy, lz, type);
    LightEngine::computeChunkLight(*c);
    c->buildMesh();

    if (lx == 0)                     if (Chunk* n = getChunk(cx - 1, cz)) n->buildMesh();
    if (lx == Chunk::SIZE_X - 1)     if (Chunk* n = getChunk(cx + 1, cz)) n->buildMesh();
    if (lz == 0)                     if (Chunk* n = getChunk(cx, cz - 1)) n->buildMesh();
    if (lz == Chunk::SIZE_Z - 1)     if (Chunk* n = getChunk(cx, cz + 1)) n->buildMesh();
}

RaycastHit World::raycast(const glm::vec3& origin, const glm::vec3& dir, float maxDist) {
    RaycastHit result;
    constexpr float STEP = 0.05f;  

    glm::ivec3 prev = glm::ivec3(glm::floor(origin));

    for (float t = 0.0f; t < maxDist; t += STEP) {
        glm::vec3 p = origin + dir * t;
        glm::ivec3 block = glm::ivec3(glm::floor(p));

        BlockType b = getBlockWorld(block.x, block.y, block.z);
        if (!isAir(b)) {
            result.hit = true;
            result.blockPos = block;
            result.prevPos = prev;
            result.blockType = b;
            return result;
        }
        prev = block;
    }

    return result;
}

static constexpr int SEA_LEVEL = 40;
static constexpr int BASE_HEIGHT = 48;
static constexpr int BEACH_MAX = 43;
static constexpr int MOUNTAIN_MIN = 70;
static constexpr int SNOW_MIN = 90;
static constexpr int TREE_RADIUS = 2;
static constexpr int TREE_CROWN_H = 4;

int World::sampleHeight(int wx, int wz) {
    float fwx = (float)wx;
    float fwz = (float)wz;

    float continental = m_noiseContinental.GetNoise(fwx, fwz);
    int baseHeight = BASE_HEIGHT + (int)(continental * 8.0f);     

    float erosionRaw = m_noiseErosion.GetNoise(fwx, fwz);
    float erosion = (erosionRaw + 1.0f) * 0.5f;                   

    float hillRaw = m_noiseHilliness.GetNoise(fwx, fwz);
    float hills = (hillRaw + 1.0f) * 0.5f;
    int hillHeight = (int)(hills * hills * 12.0f);                

    float mountainRaw = m_noiseMountains.GetNoise(fwx, fwz);
    float mountains = std::max(0.0f, mountainRaw);
    mountains = std::pow(mountains, 1.5f);
    float mountainMask = std::max(0.0f, erosion - 0.35f) / 0.65f;
    mountainMask = mountainMask * mountainMask;
    int mountainHeight = (int)(mountains * mountainMask * 95.0f); 

    float detail = m_noiseDetail.GetNoise(fwx, fwz);
    int detailHeight = (int)(detail * 2.0f);                      

    int height = baseHeight + hillHeight + mountainHeight + detailHeight;

    if (height < 1) height = 1;
    if (height >= Chunk::SIZE_Y) height = Chunk::SIZE_Y - 1;
    return height;
}

BlockType World::sampleTopBlock(int height) {
    if (height >= SNOW_MIN) return BlockType::Snow;
    if (height >= MOUNTAIN_MIN) return BlockType::Stone;
    if (height <= BEACH_MAX) return BlockType::Sand;
    return BlockType::Grass;
}

bool World::shouldSpawnTreeCandidate(int wx, int wz) {
    int h = sampleHeight(wx, wz);
    if (h <= SEA_LEVEL + 1) return false;
    if (sampleTopBlock(h) != BlockType::Grass) return false;
    return rand01(wx, wz, (uint32_t)m_seed) < 0.012f;
}

bool World::shouldSpawnTree(int wx, int wz) {
    int h = sampleHeight(wx, wz);
    if (h <= SEA_LEVEL + 1) return false;
    if (sampleTopBlock(h) != BlockType::Grass) return false;

    if (rand01(wx, wz, (uint32_t)m_seed) >= 0.012f) return false;

    uint32_t myH = hash21(wx, wz, (uint32_t)m_seed + 1337u);
    for (int dz = -2; dz <= 2; dz++) {
        for (int dx = -2; dx <= 2;dx++) {
            if (dx == 0 && dz == 0) continue;
            if (!shouldSpawnTreeCandidate(wx + dx, wz + dz)) continue; // tania pre-check
            uint32_t h2 = hash21(wx + dx, wz + dz, (uint32_t)m_seed + 1337u);
            if (h2 <= myH) return false;
        }
    }
    return true;
}

void World::update(const glm::vec3& cameraPos, int renderDistance) {
    int camCx, camCz, lx, lz;
    chunkCoordsFromWorld((int)std::floor(cameraPos.x), (int)std::floor(cameraPos.z),
        camCx, camCz, lx, lz);
    {
        std::lock_guard<std::mutex> lk(m_queueMutex);

        while (m_requestQueue.size() < 16) {
            int bestDist = INT_MAX;
            glm::ivec2 bestPos{};
            bool foundMissing = false;

            for (int dz = -renderDistance; dz <= renderDistance; dz++) {
                for (int dx = -renderDistance; dx <= renderDistance; dx++) {
                    int cx = camCx + dx;
                    int cz = camCz + dz;
                    int dist = dx * dx + dz * dz;
                    if (dist > renderDistance * renderDistance) continue;

                    glm::ivec2 pos{ cx, cz };
                    if (m_chunks.count(pos)) continue;
                    if (m_inFlight.count(pos)) continue;

                    if (dist < bestDist) {
                        bestDist = dist;
                        bestPos = pos;
                        foundMissing = true;
                    }
                }
            }

            if (!foundMissing) break;
            m_inFlight.insert(bestPos);
            m_requestQueue.push(bestPos);
        }
    }
    m_cv.notify_all();

    int meshedThisFrame = 0;
    while (meshedThisFrame < MAX_MESH_UPLOADS_PER_FRAME) {
        std::unique_ptr<Chunk> chunk;
        glm::ivec2 pos;
        {
            std::lock_guard<std::mutex> lk(m_queueMutex);
            if (m_readyQueue.empty()) break;
            chunk = std::move(m_readyQueue.front());
            m_readyQueue.pop();
            pos = { chunk->chunkPos().x, chunk->chunkPos().z };
            m_inFlight.erase(pos);
        }

        int dx = pos.x - camCx;
        int dz = pos.y - camCz;
        if (dx * dx + dz * dz > (renderDistance + 1) * (renderDistance + 1)) {
            continue;
        }

        chunk->setWorld(this);
        chunk->buildMesh();
        m_chunks[pos] = std::move(chunk);
        meshedThisFrame++;

        if (getChunk(pos.x - 1, pos.y)) m_dirtyMeshes.insert({ pos.x - 1, pos.y });
        if (getChunk(pos.x + 1, pos.y)) m_dirtyMeshes.insert({ pos.x + 1, pos.y });
        if (getChunk(pos.x, pos.y - 1)) m_dirtyMeshes.insert({ pos.x,     pos.y - 1 });
        if (getChunk(pos.x, pos.y + 1)) m_dirtyMeshes.insert({ pos.x,     pos.y + 1 });
    }

    int unloadedThisFrame = 0;
    std::vector<glm::ivec2> toRemove;
    for (auto& [pos, chunk] : m_chunks) {
        int dx = pos.x - camCx;
        int dz = pos.y - camCz;
        int dist = dx * dx + dz * dz;
        if (dist > (renderDistance + 1) * (renderDistance + 1)) {
            toRemove.push_back(pos);
            if ((int)toRemove.size() >= MAX_CHUNKS_UNLOADED_PER_FRAME) break;
        }
    }
    for (auto& p : toRemove) {
        m_chunks.erase(p);
        unloadedThisFrame++;

        if (Chunk* n = getChunk(p.x - 1, p.y)) m_dirtyMeshes.insert({ p.x - 1, p.y });
        if (Chunk* n = getChunk(p.x + 1, p.y)) m_dirtyMeshes.insert({ p.x + 1, p.y });
        if (Chunk* n = getChunk(p.x, p.y - 1)) m_dirtyMeshes.insert({ p.x,     p.y - 1 });
        if (Chunk* n = getChunk(p.x, p.y + 1)) m_dirtyMeshes.insert({ p.x,     p.y + 1 });
    }

	int rebuiltThisFrame = 0;
	auto it = m_dirtyMeshes.begin();
    while (it != m_dirtyMeshes.end() && rebuiltThisFrame < MAX_DIRTY_REBUILDS_PER_FRAME) {
        if (Chunk* c = getChunk(it->x, it->y)) {
            c->buildMesh();
            rebuiltThisFrame++;
        }
        it = m_dirtyMeshes.erase(it);
    }
}

void World::draw(Shader& shader) const {
    for (const auto& [pos, chunk] : m_chunks) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), chunk->worldPos());
        shader.setMat4("uModel", model);
        chunk->draw();
    }
}

void World::placeTreeAt(Chunk& chunk, int wx, int wz, int baseY) {
    const glm::ivec3 cp = chunk.chunkPos();
    const int chunkOriginX = cp.x * Chunk::SIZE_X;
    const int chunkOriginZ = cp.z * Chunk::SIZE_Z;

    int trunkH = 4 + (hash21(wx, wz, (uint32_t)m_seed + 99u) % 3);
    if (baseY + trunkH + TREE_CROWN_H >= Chunk::SIZE_Y) return;

    auto tryPlace = [&](int wxx, int wy, int wzz, BlockType t, bool overwrite) {
        int lx = wxx - chunkOriginX;
        int lz = wzz - chunkOriginZ;
        if (lx < 0 || lx >= Chunk::SIZE_X) return; 
        if (lz < 0 || lz >= Chunk::SIZE_Z) return;
        if (wy < 0 || wy >= Chunk::SIZE_Y) return;
        BlockType cur = chunk.getBlock(lx, wy, lz);
        if (!overwrite && cur != BlockType::Air && cur != BlockType::Leaves) return;
        chunk.setBlock(lx, wy, lz, t);
        };

    for (int i = 1; i <= trunkH; i++) {
        tryPlace(wx, baseY + i, wz, BlockType::Wood, true);
    }

    int topY = baseY + trunkH;
    for (int layer = 0; layer < TREE_CROWN_H; layer++) {
        int y = topY + layer - 1;
        int r = (layer <= 1) ? 2 : 1;
        for (int dz = -r; dz <= r; dz++) {
            for (int dx = -r; dx <= r; dx++) {
                bool corner = (std::abs(dx) == r && std::abs(dz) == r);
                if (layer <= 1 && corner) {
                    uint32_t hh = hash21(wx + dx, wz + dz, (uint32_t)m_seed + 77u + layer);
                    if ((hh & 1u) == 0u) continue;
                }
                if (layer == 3 && (std::abs(dx) + std::abs(dz) > 1)) continue;
                if (dx == 0 && dz == 0 && layer < 2) continue;
                tryPlace(wx + dx, y, wz + dz, BlockType::Leaves, false);
            }
        }
    }
}

void World::generateChunkTerrain(Chunk& chunk) {
    glm::ivec3 cp = chunk.chunkPos();
    int worldOffsetX = cp.x * Chunk::SIZE_X;
    int worldOffsetZ = cp.z * Chunk::SIZE_Z;

    int heights[Chunk::SIZE_X][Chunk::SIZE_Z];

    for (int x = 0; x < Chunk::SIZE_X; x++) {
        for (int z = 0; z < Chunk::SIZE_Z; z++) {
            int wx = worldOffsetX + x;
            int wz = worldOffsetZ + z;

            int height = sampleHeight(wx, wz);
            heights[x][z] = height;

            BlockType topBlock = sampleTopBlock(height);
            BlockType subBlock = (topBlock == BlockType::Grass) ? BlockType::Dirt
                : (topBlock == BlockType::Sand) ? BlockType::Sand
                : BlockType::Stone;

            for (int y = 0; y < Chunk::SIZE_Y; y++) {
                BlockType b;
                if (y < height - 4)        b = BlockType::Stone;
                else if (y < height)       b = subBlock;
                else if (y == height)      b = topBlock;
                else if (y <= SEA_LEVEL)   b = BlockType::Water;
                else                       b = BlockType::Air;

                // === KLIFY / NAWISY ===
                // W gorach (height > 58), do 16 blokow w glab
                if (height > 58 && y >= height - 16 && y <= height &&
                    b != BlockType::Air && b != BlockType::Water && y > SEA_LEVEL)
                {
                    float cn = m_noiseCliff3D.GetNoise((float)wx, (float)y * 1.0f, (float)wz);
                    float depth = (float)(height - y) / 16.0f;          // 0..1
                    float strength = std::min(1.0f, (height - 58) / 40.0f);
                    float threshold = 0.35f + depth * 0.35f - strength * 0.15f;
                    if (cn > threshold) {
                        b = BlockType::Air;
                    }
                }

                chunk.setBlock(x, y, z, b);
            }
        }
    }

    m_caveGen.carveChunk(chunk, heights);
    m_oreGen.generateOres(chunk);
    m_oreGen.generateBedrock(chunk);
}

void World::generateChunkTrees(Chunk& chunk) {
    glm::ivec3 cp = chunk.chunkPos();
    int worldOffsetX = cp.x * Chunk::SIZE_X;
    int worldOffsetZ = cp.z * Chunk::SIZE_Z;

    for (int x = -TREE_RADIUS; x < Chunk::SIZE_X + TREE_RADIUS; x++) {
        for (int z = -TREE_RADIUS; z < Chunk::SIZE_Z + TREE_RADIUS; z++) {
            int wx = worldOffsetX + x;
            int wz = worldOffsetZ + z;

            if (!shouldSpawnTree(wx, wz)) continue;

            int baseY = sampleHeight(wx, wz);
            placeTreeAt(chunk, wx, wz, baseY);
        }
    }
}

void World::workerLoop() {
    while (true) {
        glm::ivec2 pos;
        {
            std::unique_lock<std::mutex> lk(m_queueMutex);
            m_cv.wait(lk, [this] { return !m_running || !m_requestQueue.empty(); });
            if (!m_running && m_requestQueue.empty()) return;
            pos = m_requestQueue.front();
            m_requestQueue.pop();
        }

        auto chunk = std::make_unique<Chunk>(glm::ivec3(pos.x, 0, pos.y));
        generateChunkTerrain(*chunk);
        generateChunkTrees(*chunk);
        LightEngine::computeChunkLight(*chunk);

        {
            std::lock_guard<std::mutex> lk(m_queueMutex);
            m_readyQueue.push(std::move(chunk));
        }
    }
}