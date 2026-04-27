#pragma once
#include "Chunk.h"

class LightEngine {
public:
    static void computeChunkLight(Chunk& chunk);

private:
    static void computeSkyLight(Chunk& chunk);
    static void computeBlockLight(Chunk& chunk);
};