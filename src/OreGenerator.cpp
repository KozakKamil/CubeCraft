#include "OreGenerator.h"
#include <cmath>
#include <algorithm>

uint64_t OreGenerator::Rng::nextU64() {
    s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
    return s * 2685821657736338717ULL;
}
int OreGenerator::Rng::nextInt(int bound) {
    if (bound <= 0) return 0;
    return (int)(nextU64() % (uint64_t)bound);
}
float OreGenerator::Rng::nextFloat() {
    return (nextU64() >> 40) / (float)(1 << 24);
}

OreGenerator::OreGenerator(int worldSeed) : m_seed(worldSeed) {}

uint64_t OreGenerator::hashChunk(int worldSeed, int cx, int cz, uint32_t salt) {
    uint64_t h = (uint64_t)(uint32_t)worldSeed * 0x9E3779B97F4A7C15ULL;
    h ^= (uint64_t)(uint32_t)cx * 0xC2B2AE3D27D4EB4FULL;
    h ^= (uint64_t)(uint32_t)cz * 0x165667B19E3779F9ULL;
    h ^= (uint64_t)salt * 0xD1B54A32D192ED03ULL;
    h ^= h >> 33; h *= 0xFF51AFD7ED558CCDULL;
    h ^= h >> 33; h *= 0xC4CEB9FE1A85EC53ULL;
    h ^= h >> 33;
    return h ? h : 0x9E3779B97F4A7C15ULL;
}

void OreGenerator::generateBedrock(Chunk& chunk) {
    const glm::ivec3 cp = chunk.chunkPos();
    Rng rng(hashChunk(m_seed, cp.x, cp.z, 0xBEDC0DE));

    for (int x = 0; x < Chunk::SIZE_X; x++) {
        for (int z = 0; z < Chunk::SIZE_Z; z++) {
            // Y=0 zawsze bedrock
            chunk.setBlock(x, 0, z, BlockType::Bedrock);
            // Y=1..4 z malejacym prawdopodobienstwem (jak alpha)
            for (int y = 1; y <= 4; y++) {
                // szansa: y=1 -> 80%, y=2 -> 60%, y=3 -> 40%, y=4 -> 20%
                int prob = (5 - y) * 20;
                if (rng.nextInt(100) < prob) {
                    BlockType cur = chunk.getBlock(x, y, z);
                    // nie zastapuj powietrza (zostaw tunele jaskin nienaruszone)
                    if (cur != BlockType::Air && cur != BlockType::Water) {
                        chunk.setBlock(x, y, z, BlockType::Bedrock);
                    }
                }
            }
        }
    }
}

void OreGenerator::carveVein(Chunk& chunk, BlockType ore,
    double cx, double cy, double cz,
    int veinSize, Rng& rng)
{
    // Vein to lancuch ~veinSize sferycznych klastrow polozonych blisko siebie.
    // Daje to wydluzony, naturalny ksztalt zamiast okraglej kuli.
    const glm::ivec3 cp = chunk.chunkPos();
    const int ox = cp.x * Chunk::SIZE_X;
    const int oz = cp.z * Chunk::SIZE_Z;

    float yaw = rng.nextFloat() * 6.2831853f;
    float pitch = (rng.nextFloat() - 0.5f) * 1.5f;

    double x = cx, y = cy, z = cz;

    for (int step = 0; step < veinSize; step++) {
        float r = 0.5f + rng.nextFloat() * 1.0f;   // promien klastra: 0.5..1.5
        // wycinamy AABB i zamieniamy stone na ore w sferze
        int minX = std::max(0, (int)std::floor(x - r) - ox);
        int maxX = std::min(Chunk::SIZE_X - 1, (int)std::floor(x + r) - ox);
        int minZ = std::max(0, (int)std::floor(z - r) - oz);
        int maxZ = std::min(Chunk::SIZE_Z - 1, (int)std::floor(z + r) - oz);
        int minY = std::max(1, (int)std::floor(y - r));
        int maxY = std::min(Chunk::SIZE_Y - 2, (int)std::floor(y + r));

        float r2 = r * r;
        for (int lx = minX; lx <= maxX; lx++) {
            double dx = (double)(ox + lx) + 0.5 - x;
            for (int lz = minZ; lz <= maxZ; lz++) {
                double dz = (double)(oz + lz) + 0.5 - z;
                for (int ly = minY; ly <= maxY; ly++) {
                    double dy = (double)ly + 0.5 - y;
                    if (dx * dx + dy * dy + dz * dz >= r2) continue;
                    if (chunk.getBlock(lx, ly, lz) == BlockType::Stone) {
                        chunk.setBlock(lx, ly, lz, ore);
                    }
                }
            }
        }

        // krok do przodu z drobnym wahaniem
        x += std::cos(yaw) * std::cos(pitch);
        z += std::sin(yaw) * std::cos(pitch);
        y += std::sin(pitch);
        yaw += (rng.nextFloat() - 0.5f) * 0.5f;
        pitch += (rng.nextFloat() - 0.5f) * 0.3f;
        pitch *= 0.9f;
    }
}

void OreGenerator::scatterVeins(Chunk& chunk, BlockType ore,
    int veinsPerChunk, int veinSize,
    int minY, int maxY,
    uint32_t salt)
{
    const glm::ivec3 cp = chunk.chunkPos();
    Rng rng(hashChunk(m_seed, cp.x, cp.z, salt));

    int ox = cp.x * Chunk::SIZE_X;
    int oz = cp.z * Chunk::SIZE_Z;

    for (int i = 0; i < veinsPerChunk; i++) {
        double x = (double)(ox + rng.nextInt(Chunk::SIZE_X));
        double z = (double)(oz + rng.nextInt(Chunk::SIZE_Z));
        double y = (double)(minY + rng.nextInt(maxY - minY + 1));
        carveVein(chunk, ore, x, y, z, veinSize, rng);
    }
}

void OreGenerator::generateOres(Chunk& chunk) {
    // Parametry sterowane wartosciami z alpha 1.1.2 (MapGenMinable):
    //   ore             veinsPerChunk  veinSize    Y range
    //   coal            ~20            16          0..127  (tu: 5..60)
    //   iron            ~20            8           0..63
    //   gold            ~2             8           0..31
    //   diamond         ~1             7           0..15
    //   redstone        ~8             7           0..15  (TODO: dodamy gdy bedzie BlockType)
    //
    // Ja zmniejszam liczby zeby nie bylo overkill na 16x16 chunku.

    scatterVeins(chunk, BlockType::CoalOre, 20, 12, 5, 60, 0xC0A1FACE);
    scatterVeins(chunk, BlockType::IronOre, 12, 8, 5, 50, 0x12012FACE);
    scatterVeins(chunk, BlockType::GoldOre, 3, 8, 5, 28, 0x90D0FACE);
    scatterVeins(chunk, BlockType::DiamondOre, 1, 7, 5, 14, 0xD1A30FACE);
}