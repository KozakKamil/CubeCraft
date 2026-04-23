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

void World::generateChunkTerrain(Chunk& chunk) {
	const int SEA_LEVEL = 40;
	const int BASE_HEIGHT = 48;
    const int BEACH_MAX = 43;
	const int MOUNTAIN_MIN = 70;
	const int SNOW_MIN = 90;

	glm::ivec3 cp = chunk.chunkPos();
	int worldOffsetX = cp.x * Chunk::SIZE_X;
	int worldOffsetZ = cp.z * Chunk::SIZE_Z;

    for(int x =0; x<Chunk::SIZE_X; x++){
        for (int z = 0; z < Chunk::SIZE_Z; z++) {
            float wx = (float)(worldOffsetX + x);
            float wz = (float)(worldOffsetZ + z);

            float continental = m_noiseContinental.GetNoise(wx, wz);
            float hillinesRaw = m_noiseHilliness.GetNoise(wx, wz);
            float detail = m_noiseDetail.GetNoise(wx, wz);

            float hilliness = (hillinesRaw + 1.0f) * 0.5f;
            hilliness = hilliness * hilliness * hilliness;

            int height = BASE_HEIGHT + (int)(continental * 18.0f) + (int)(hilliness * detail * 40.0f);

            if (height < 1) height = 1;
            if (height >= Chunk::SIZE_Y) height = Chunk::SIZE_Y - 1;

            BlockType topBlock, subBlock;
            if (height >= SNOW_MIN) { topBlock = BlockType::Snow; subBlock = BlockType::Stone; }
            else if (height >= MOUNTAIN_MIN) { topBlock = BlockType::Stone; subBlock = BlockType::Stone; }
            else if (height <= BEACH_MAX) { topBlock = BlockType::Sand; subBlock = BlockType::Sand; }
            else { topBlock = BlockType::Grass; subBlock = BlockType::Dirt; }

            for (int y = 0; y < Chunk::SIZE_Y; y++) {
                if (y < height - 4) chunk.setBlock(x, y, z, BlockType::Stone);
                else if (y < height) chunk.setBlock(x, y, z, subBlock);
                else if (y == height) chunk.setBlock(x, y, z, topBlock);
                else if (y <= SEA_LEVEL) chunk.setBlock(x, y, z, BlockType::Water);
                else chunk.setBlock(x, y, z, BlockType::Air);
            }

			int worldX = worldOffsetX + x;
			int worldZ = worldOffsetZ + z;

            if (topBlock == BlockType::Grass && height > SEA_LEVEL + 1) {
                float r = rand01(worldX, worldZ, (uint32_t)m_seed);

                if (r < 0.035f) {
                    int trunkH = 4 + (hash21(worldX, worldZ, (uint32_t)m_seed + 99) % 3);
					int topY = height;

					if (topY + trunkH + 3 < Chunk::SIZE_Y) {
                        for(int i = 1; i <= trunkH; i++){
							chunk.setBlock(x, topY + i, z, BlockType::Wood);
                        }

						int crownY = topY + trunkH;
                        for(int dy = -2; dy <=2; dy++){
                            for(int dx = -2; dx <=2; dx++){
                                for (int dz = -2; dz <= 2; dz++) {
                                    int ax = x + dx;
									int ay = crownY + dy;
                                    int az = z + dz;

									int dist2 = dx * dx + dz * dz + dy * dy;

                                    if (dist2 > 6) continue;

									if (dx == 0 && dz == 0 && dy <= 0) continue;

									chunk.setBlock(ax, ay, az, BlockType::Leaves);
                                }
                            }
                        }
                    }
                }
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