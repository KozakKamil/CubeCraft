#pragma once
#include <cstdint>

enum class BlockType : uint8_t {
	Air = 0,
	Grass,
	Dirt,
	Stone,
	Sand,
	Snow,
	Water
};

inline bool isAir(BlockType b) {
	return b == BlockType::Air;
}

inline bool isTransparent(BlockType b) {
	return b == BlockType::Air || b == BlockType::Water;
}