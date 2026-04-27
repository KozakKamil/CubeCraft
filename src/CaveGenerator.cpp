#include "CaveGenerator.h"
#include <cmath>
#include <algorithm>

namespace {

    constexpr double PI = 3.14159265358979323846;
    constexpr int TOP_PROTECT = 5;

    void carveSphere(Chunk& chunk,
        const int heights[Chunk::SIZE_X][Chunk::SIZE_Z],
        double cx, double cy, double cz, float rxz, float ry)
    {
        const glm::ivec3 cp = chunk.chunkPos();
        const int ox = cp.x * Chunk::SIZE_X;
        const int oz = cp.z * Chunk::SIZE_Z;

        int minX = std::max(0, (int)std::floor(cx - rxz) - ox);
        int maxX = std::min(Chunk::SIZE_X - 1, (int)std::floor(cx + rxz) - ox);
        int minZ = std::max(0, (int)std::floor(cz - rxz) - oz);
        int maxZ = std::min(Chunk::SIZE_Z - 1, (int)std::floor(cz + rxz) - oz);
        int minY = std::max(1, (int)std::floor(cy - ry));
        int maxY = std::min(Chunk::SIZE_Y - 2, (int)std::floor(cy + ry));

        if (minX > maxX || minZ > maxZ || minY > maxY) return;

        // KROK 1: Znajdz NAJNIZSZA powierzchnie w obszarze sfery.
        // Jezeli sfera siegnelaby blizej niz TOP_PROTECT do tej powierzchni,
        // CALA sfera jest odrzucona (nie tylko jej wierzch).
        int minSurface = Chunk::SIZE_Y;
        for (int lx = minX; lx <= maxX; lx++) {
            for (int lz = minZ; lz <= maxZ; lz++) {
                if (heights[lx][lz] < minSurface) minSurface = heights[lx][lz];
            }
        }
        // jezeli sfera by przebila trawe -> odrzuc cala kule
        if (maxY > minSurface - TOP_PROTECT) return;

        const float rxz2 = rxz * rxz;
        const float ry2 = ry * ry;

        for (int lx = minX; lx <= maxX; lx++) {
            double dx = (double)(ox + lx) + 0.5 - cx;
            for (int lz = minZ; lz <= maxZ; lz++) {
                double dz = (double)(oz + lz) + 0.5 - cz;
                for (int ly = minY; ly <= maxY; ly++) {
                    double dy = (double)ly + 0.5 - cy;
                    double e = (dx * dx) / rxz2 + (dy * dy) / ry2 + (dz * dz) / rxz2;
                    if (e >= 1.0) continue;

                    BlockType b = chunk.getBlock(lx, ly, lz);
                    if (b == BlockType::Stone || b == BlockType::Dirt ||
                        b == BlockType::Grass || b == BlockType::Sand)
                    {
                        chunk.setBlock(lx, ly, lz, BlockType::Air);
                    }
                }
            }
        }
    }

} // namespace

uint64_t CaveGenerator::Rng::nextU64() {
    s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
    return s * 2685821657736338717ULL;
}
int CaveGenerator::Rng::nextInt(int bound) {
    if (bound <= 0) return 0;
    return (int)(nextU64() % (uint64_t)bound);
}
float CaveGenerator::Rng::nextFloat() {
    return (nextU64() >> 40) / (float)(1 << 24);
}

CaveGenerator::CaveGenerator(int worldSeed) : m_seed(worldSeed) {}

uint64_t CaveGenerator::hashChunk(int worldSeed, int cx, int cz) {
    uint64_t h = (uint64_t)(uint32_t)worldSeed * 0x9E3779B97F4A7C15ULL;
    h ^= (uint64_t)(uint32_t)cx * 0xC2B2AE3D27D4EB4FULL;
    h ^= (uint64_t)(uint32_t)cz * 0x165667B19E3779F9ULL;
    h ^= h >> 33; h *= 0xFF51AFD7ED558CCDULL;
    h ^= h >> 33; h *= 0xC4CEB9FE1A85EC53ULL;
    h ^= h >> 33;
    return h ? h : 0x9E3779B97F4A7C15ULL;
}

void CaveGenerator::carveChunk(Chunk& chunk,
    const int heights[Chunk::SIZE_X][Chunk::SIZE_Z])
{
    const glm::ivec3 cp = chunk.chunkPos();
    constexpr int RANGE = 8;
    for (int dz = -RANGE; dz <= RANGE; dz++) {
        for (int dx = -RANGE; dx <= RANGE; dx++) {
            emitCavesFromOrigin(chunk, heights, cp.x + dx, cp.z + dz);
        }
    }
}

void CaveGenerator::emitCavesFromOrigin(Chunk& target,
    const int heights[Chunk::SIZE_X][Chunk::SIZE_Z],
    int ocx, int ocz)
{
    Rng rng(hashChunk(m_seed, ocx, ocz));

    int caveCount = rng.nextInt(rng.nextInt(rng.nextInt(10) + 1) + 1);
    if (rng.nextInt(7) != 0) caveCount = 0;

    for (int i = 0; i < caveCount; i++) {
        double x = (double)(ocx * Chunk::SIZE_X) + rng.nextInt(Chunk::SIZE_X);
        // ZMIANA: znacznie nizszy start Y (bias w okolice y=10..35)
        double y = (double)rng.nextInt(rng.nextInt(50) + 8);
        double z = (double)(ocz * Chunk::SIZE_Z) + rng.nextInt(Chunk::SIZE_Z);

        int branches = 1;

        if (rng.nextInt(4) == 0) {
            float roomR = 1.0f + rng.nextFloat() * 6.0f;
            carveTunnel(target, heights, x, y, z, roomR, 0.0f, 0.0f, 0, rng, true);
            branches += rng.nextInt(4);
        }

        for (int b = 0; b < branches; b++) {
            float yaw = rng.nextFloat() * (float)PI * 2.0f;
            float pitch = (rng.nextFloat() - 0.5f) * 0.25f;
            float radius = rng.nextFloat() * 2.0f + 1.5f;
            if (rng.nextInt(10) == 0) {
                radius *= rng.nextFloat() * rng.nextFloat() * 3.0f + 1.0f;
            }
            int length = 400 + rng.nextInt(rng.nextInt(400) + 1);
            carveTunnel(target, heights, x, y, z, radius, yaw, pitch, length, rng, false);
        }
    }
}

void CaveGenerator::carveTunnel(Chunk& target,
    const int heights[Chunk::SIZE_X][Chunk::SIZE_Z],
    double x, double y, double z,
    float baseRadius,
    float yaw, float pitch,
    int totalSteps,
    Rng& rng,
    bool wideRoom)
{
    if (wideRoom) {
        carveSphere(target, heights, x, y, z, baseRadius * 1.5f, baseRadius);
        return;
    }
    if (totalSteps <= 0) return;

    const glm::ivec3 cp = target.chunkPos();
    const double chunkCx = cp.x * Chunk::SIZE_X + Chunk::SIZE_X * 0.5;
    const double chunkCz = cp.z * Chunk::SIZE_Z + Chunk::SIZE_Z * 0.5;

    float yawDelta = 0.0f;
    float pitchDelta = 0.0f;

    for (int i = 0; i < totalSteps; i++) {
        double prog = (double)i / (double)totalSteps;
        float r = baseRadius * (float)(0.5 + std::sin(prog * PI) * 0.75);
        if (r < 1.5f) r = 1.5f;
        float ry = r * 0.7f + 0.5f;

        double cosP = std::cos(pitch);
        x += std::cos(yaw) * cosP;
        z += std::sin(yaw) * cosP;
        y += std::sin(pitch);

        pitch *= 0.7f;
        pitch += pitchDelta * 0.05f;
        yaw += yawDelta * 0.05f;
        pitchDelta = pitchDelta * 0.75f + (rng.nextFloat() - rng.nextFloat()) * 2.0f;
        yawDelta = yawDelta * 0.75f + (rng.nextFloat() - rng.nextFloat()) * 4.0f;

        // Hard cap - jaskinie tylko w dolnej polowie swiata
        if (y < 1 || y > 70) continue;

        double dx = x - chunkCx;
        double dz = z - chunkCz;
        double remain = (double)(totalSteps - i);
        double maxReach = remain + r + 16.0;
        if (dx * dx + dz * dz > maxReach * maxReach) return;

        carveSphere(target, heights, x, y, z, r, ry);
    }
}