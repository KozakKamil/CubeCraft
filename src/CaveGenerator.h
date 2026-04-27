#pragma once
#include "Chunk.h"
#include <cstdint>

class CaveGenerator {
public:
    explicit CaveGenerator(int worldSeed);

    // heights[lx][lz] = sampleHeight dla kazdej kolumny w chunku
    void carveChunk(Chunk& chunk, const int heights[Chunk::SIZE_X][Chunk::SIZE_Z]);

private:
    struct Rng {
        uint64_t s;
        explicit Rng(uint64_t seed) : s(seed ? seed : 0x9E3779B97F4A7C15ULL) {}
        uint64_t nextU64();
        int      nextInt(int bound);
        float    nextFloat();
    };

    int m_seed;

    static uint64_t hashChunk(int worldSeed, int cx, int cz);

    void emitCavesFromOrigin(Chunk& target,
        const int heights[Chunk::SIZE_X][Chunk::SIZE_Z],
        int ocx, int ocz);

    void carveTunnel(Chunk& target,
        const int heights[Chunk::SIZE_X][Chunk::SIZE_Z],
        double x, double y, double z,
        float baseRadius,
        float yaw, float pitch,
        int totalSteps,
        Rng& rng,
        bool wideRoom);
};