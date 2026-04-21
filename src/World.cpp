#include "World.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>

World::World(int seed) : m_seed(seed) {
    m_noise.SetSeed(seed);
    m_noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noise.SetFrequency(0.02f);
    m_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noise.SetFractalOctaves(4);
    m_noise.SetFractalLacunarity(2.0f);
    m_noise.SetFractalGain(0.5f);
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

void World::generateChunkTerrain(Chunk& chunk) {
    constexpr int SEA_LEVEL = 8;  
    constexpr int BEACH_MAX = 16;  
    constexpr int MOUNTAIN_MIN = 18;
    constexpr int SNOW_MIN = 24;

    glm::ivec3 cp = chunk.chunkPos();
    int worldOffsetX = cp.x * Chunk::SIZE_X;
    int worldOffsetZ = cp.z * Chunk::SIZE_Z;

    for (int x = 0; x < Chunk::SIZE_X; x++) {
        for (int z = 0; z < Chunk::SIZE_Z; z++) {
            float wx = (float)(worldOffsetX + x);
            float wz = (float)(worldOffsetZ + z);
            float n = m_noise.GetNoise(wx, wz);

            int height = 16 + (int)(n * 12.0f);
            if (height < 1) height = 1;
            if (height >= Chunk::SIZE_Y) height = Chunk::SIZE_Y - 1;

            BlockType topBlock;
            BlockType subBlock;
            if (height >= SNOW_MIN) { topBlock = BlockType::Snow;  subBlock = BlockType::Stone; }
            else if (height >= MOUNTAIN_MIN) { topBlock = BlockType::Stone; subBlock = BlockType::Stone; }
            else if (height <= BEACH_MAX) { topBlock = BlockType::Sand;  subBlock = BlockType::Sand; }
            else { topBlock = BlockType::Grass; subBlock = BlockType::Dirt; }

            for (int y = 0; y < Chunk::SIZE_Y; y++) {
                if (y < height - 3)      chunk.setBlock(x, y, z, BlockType::Stone);
                else if (y < height)     chunk.setBlock(x, y, z, subBlock);
                else if (y == height)    chunk.setBlock(x, y, z, topBlock);
                else if (y <= SEA_LEVEL) chunk.setBlock(x, y, z, BlockType::Water);
                else                     chunk.setBlock(x, y, z, BlockType::Air);
            }
        }
    }
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
        generateChunkTerrain(*chunk);
        chunk->buildMesh();
        m_chunks[bestPos] = std::move(chunk);
        loadedThisFrame++;
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
    }
}

void World::draw(Shader& shader) const {
    for (const auto& [pos, chunk] : m_chunks) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), chunk->worldPos());
        shader.setMat4("uModel", model);
        chunk->draw();
    }
}