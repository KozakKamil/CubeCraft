#pragma once
#include <cstdint>

enum class BlockType : uint8_t {
	Air = 0,
	Grass,
	Dirt,
	Stone,
};

inline bool isAir(BlockType b) {
	return b == BlockType::Air;
}
