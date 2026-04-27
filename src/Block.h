#pragma once
#include <cstdint>

enum class BlockType : uint8_t {
    Air = 0,
    Grass,
    Dirt,
    Stone,
    Sand,
    Snow,
    Water,
    Wood,
    Leaves,
    CoalOre,
    IronOre,
    GoldOre,
    DiamondOre,
    Bedrock,
    Torch          // <-- DODAJ
};

inline bool isAir(BlockType b) {
    return b == BlockType::Air;
}

inline bool isTransparent(BlockType b) {
    return b == BlockType::Air
        || b == BlockType::Water
        || b == BlockType::Leaves
        || b == BlockType::Torch;
}

inline uint8_t lightEmission(BlockType b) {
    switch (b) {
    case BlockType::Torch: 
        return 14;
    default:               
        return 0;
    }
}


inline uint8_t lightOpacity(BlockType b) {
    if (b == BlockType::Air || b == BlockType::Torch) return 0;
    if (b == BlockType::Water)  return 2;
    if (b == BlockType::Leaves) return 1;
    return 15;
}