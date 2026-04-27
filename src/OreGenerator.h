#pragma once
#include "Chunk.h"
#include <cstdint>

class OreGenerator {
public:
    explicit OreGenerator(int worldSeed);

    void generateBedrock(Chunk& chunk);
    void generateOres(Chunk& chunk);

private:
    struct Rng {
        uint64_t s;
        explicit Rng(uint64_t seed) : s(seed ? seed : 0x9E3779B97F4A7C15ULL) {}
        uint64_t nextU64();
        int      nextInt(int bound);
        float    nextFloat();
    };

    int m_seed;

    static uint64_t hashChunk(int worldSeed, int cx, int cz, uint32_t salt);

    void carveVein(Chunk& chunk, BlockType ore,
        double cx, double cy, double cz,
        int veinSize, Rng& rng);

    void scatterVeins(Chunk& chunk, BlockType ore,
        int veinsPerChunk, int veinSize,
        int minY, int maxY,
        uint32_t salt);
};