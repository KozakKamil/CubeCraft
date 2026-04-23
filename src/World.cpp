#include "World.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>
#include <cstdint>

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

World::World(int seed) : m_seed(seed) {
    // 1. CONTINENTAL - wielkie kontynenty/oceany (niska freq)
    m_noiseContinental.SetSeed(seed);
    m_noiseContinental.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseContinental.SetFrequency(0.005f);
    m_noiseContinental.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseContinental.SetFractalOctaves(3);

    // 2. HILLINESS - selektor plasko/gorzysto (bardzo niska freq - wielkie regiony)
    m_noiseHilliness.SetSeed(seed + 1);
    m_noiseHilliness.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseHilliness.SetFrequency(0.003f);
    m_noiseHilliness.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseHilliness.SetFractalOctaves(2);

    // 3. DETAIL - ostre szczegoly gor (wysoka freq)
    m_noiseDetail.SetSeed(seed + 2);
    m_noiseDetail.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseDetail.SetFrequency(0.025f);
    m_noiseDetail.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseDetail.SetFractalOctaves(5);
    m_noiseDetail.SetFractalLacunarity(2.0f);
    m_noiseDetail.SetFractalGain(0.55f);
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
	float hillinesRaw = m_noiseHilliness.GetNoise(fwx, fwz);
    float detail = m_noiseDetail.GetNoise(fwx, fwz);

    float hillness = (hillinesRaw + 1.0f) * 0.5f;
    hillness = hillness * hillness * hillness;

    int height = BASE_HEIGHT + (int)(continental * 10.0f) + (int)(hillness * detail * 40.0f);
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

    
    int loadedThisFrame = 0;
    int bestDist = INT_MAX;
    glm::ivec2 bestPos;
    bool foundMissing = false;

    
    for (int attempt = 0; attempt < MAX_CHUNKS_LOADED_PER_FRAME; attempt++) {
        bestDist = INT_MAX;
        foundMissing = false;

        for (int dz = -renderDistance; dz <= renderDistance; dz++) {
            for (int dx = -renderDistance; dx <= renderDistance; dx++) {
                int cx = camCx + dx;
                int cz = camCz + dz;
                int dist = dx * dx + dz * dz;
                if (dist > renderDistance * renderDistance) continue;

                if (m_chunks.find({ cx, cz }) == m_chunks.end() && dist < bestDist) {
                    bestDist = dist;
                    bestPos = { cx, cz };
                    foundMissing = true;
                }
            }
        }

        if (!foundMissing) break;

        
        auto chunk = std::make_unique<Chunk>(glm::ivec3(bestPos.x, 0, bestPos.y));
		chunk->setWorld(this);
        generateChunkTerrain(*chunk);
        generateChunkTrees(*chunk);
        chunk->buildMesh();
        m_chunks[bestPos] = std::move(chunk);
        loadedThisFrame++;

		if (Chunk* n = getChunk(bestPos.x - 1, bestPos.y)) n->buildMesh();
        if (Chunk* n = getChunk(bestPos.x + 1, bestPos.y)) n->buildMesh();
        if (Chunk* n = getChunk(bestPos.x, bestPos.y - 1)) n->buildMesh();
        if (Chunk* n = getChunk(bestPos.x, bestPos.y + 1)) n->buildMesh();
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

		if (Chunk* n = getChunk(p.x - 1, p.y)) n->buildMesh();
        if (Chunk* n = getChunk(p.x + 1, p.y)) n->buildMesh();
        if (Chunk* n = getChunk(p.x, p.y - 1)) n->buildMesh();
        if (Chunk* n = getChunk(p.x, p.y + 1)) n->buildMesh();
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

    for (int x = 0; x < Chunk::SIZE_X; x++) {
        for (int z = 0; z < Chunk::SIZE_Z; z++) {
            int wx = worldOffsetX + x;
            int wz = worldOffsetZ + z;

            int height = sampleHeight(wx, wz);
            BlockType topBlock = sampleTopBlock(height);
            BlockType subBlock = (topBlock == BlockType::Grass) ? BlockType::Dirt
                : (topBlock == BlockType::Sand) ? BlockType::Sand
                : BlockType::Stone;

            for (int y = 0; y < Chunk::SIZE_Y; y++) {
                if (y < height - 4)        chunk.setBlock(x, y, z, BlockType::Stone);
                else if (y < height)       chunk.setBlock(x, y, z, subBlock);
                else if (y == height)      chunk.setBlock(x, y, z, topBlock);
                else if (y <= SEA_LEVEL)   chunk.setBlock(x, y, z, BlockType::Water);
                else                       chunk.setBlock(x, y, z, BlockType::Air);
            }
        }
    }
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