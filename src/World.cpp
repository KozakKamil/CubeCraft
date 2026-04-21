#include "World.h"
#include "Shader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

World::World(int seed) : m_seed(seed) {
    m_noise.SetSeed(seed);
    m_noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noise.SetFrequency(0.01f);         
    m_noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noise.SetFractalOctaves(8);        
    m_noise.SetFractalLacunarity(2.0f);
    m_noise.SetFractalGain(0.5f);
}

Chunk* World::getChunk(int cx, int cz) {
    auto it = m_chunks.find({ cx, cz });
    return (it != m_chunks.end()) ? it->second.get() : nullptr;
}

BlockType World::getBlockWorld(int wx, int wy, int wz) {
    int cx = (wx >= 0) ? wx / Chunk::SIZE_X : (wx - Chunk::SIZE_X + 1) / Chunk::SIZE_X;
    int cz = (wz >= 0) ? wz / Chunk::SIZE_Z : (wz - Chunk::SIZE_Z + 1) / Chunk::SIZE_Z;
    Chunk* c = getChunk(cx, cz);
    if (!c) return BlockType::Air;

    int lx = wx - cx * Chunk::SIZE_X;
    int lz = wz - cz * Chunk::SIZE_Z;
    return c->getBlock(lx, wy, lz);
}

void World::generateChunkTerrain(Chunk& chunk) {
    glm::ivec3 cp = chunk.chunkPos();
    int worldOffsetX = cp.x * Chunk::SIZE_X;
    int worldOffsetZ = cp.z * Chunk::SIZE_Z;

    for (int x = 0; x < Chunk::SIZE_X; x++) {
        for (int z = 0; z < Chunk::SIZE_Z; z++) {
            float wx = (float)(worldOffsetX + x);
            float wz = (float)(worldOffsetZ + z);

            float n = m_noise.GetNoise(wx, wz);

            int height = 16 + (int)(n * 25.0f);
            if (height < 1) height = 1;
            if (height >= Chunk::SIZE_Y) height = Chunk::SIZE_Y - 1;

            for (int y = 0; y < Chunk::SIZE_Y; y++) {
                if (y < height - 3)       chunk.setBlock(x, y, z, BlockType::Stone);
                else if (y < height)      chunk.setBlock(x, y, z, BlockType::Dirt);
                else if (y == height)     chunk.setBlock(x, y, z, BlockType::Grass);
                else                      chunk.setBlock(x, y, z, BlockType::Air);
            }
        }
    }
}

void World::generateInitial(int radius) {
    for (int cx = -radius; cx <= radius; cx++) {
        for (int cz = -radius; cz <= radius; cz++) {
            auto chunk = std::make_unique<Chunk>(glm::ivec3(cx, 0, cz));
            generateChunkTerrain(*chunk);
            m_chunks[{cx, cz}] = std::move(chunk);
        }
    }

    for (auto& [pos, chunk] : m_chunks) {
        chunk->buildMesh();
    }

    std::cout << "Wygenerowano " << m_chunks.size() << " chunkow\n";
}

void World::draw(Shader& shader) const {
    for (const auto& [pos, chunk] : m_chunks) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), chunk->worldPos());
        shader.setMat4("uModel", model);
        chunk->draw();
    }
}