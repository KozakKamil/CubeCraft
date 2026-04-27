#include "LightEngine.h"
#include <queue>
#include <tuple>
#include <algorithm>
#include <cstring>

void LightEngine::computeChunkLight(Chunk& chunk) {
    for (int x = 0; x < Chunk::SIZE_X; x++)
        for (int y = 0; y < Chunk::SIZE_Y; y++)
            for (int z = 0; z < Chunk::SIZE_Z; z++) {
                chunk.setSkyLight(x, y, z, 0);
                chunk.setBlockLight(x, y, z, 0);
            }
    computeSkyLight(chunk);
    computeBlockLight(chunk);
}

void LightEngine::computeSkyLight(Chunk& chunk) {
    using Node = std::tuple<int, int, int>;
    std::queue<Node> q;

    for (int x = 0; x < Chunk::SIZE_X; x++) {
        for (int z = 0; z < Chunk::SIZE_Z; z++) {
            int light = 15;
            for (int y = Chunk::SIZE_Y - 1; y >= 0; y--) {
                BlockType b = chunk.getBlock(x, y, z);
                int op = lightOpacity(b);
                light -= op;
                if (light <= 0) { light = 0; break; }
                chunk.setSkyLight(x, y, z, (uint8_t)light);
                if (light == 15) q.push({ x, y, z });   
            }
        }
    }

    static const int dx[6] = { 1,-1, 0, 0, 0, 0 };
    static const int dy[6] = { 0, 0, 1,-1, 0, 0 };
    static const int dz[6] = { 0, 0, 0, 0, 1,-1 };

    while (!q.empty()) q.pop();
    for (int x = 0; x < Chunk::SIZE_X; x++)
        for (int y = 0; y < Chunk::SIZE_Y; y++)
            for (int z = 0; z < Chunk::SIZE_Z; z++)
                if (chunk.getSkyLight(x, y, z) > 0)
                    q.push({ x, y, z });

    while (!q.empty()) {
        auto [cx, cy, cz] = q.front(); q.pop();
        uint8_t cur = chunk.getSkyLight(cx, cy, cz);
        if (cur <= 1) continue;

        for (int i = 0; i < 6; i++) {
            int nx = cx + dx[i], ny = cy + dy[i], nz = cz + dz[i];
            if (nx < 0 || nx >= Chunk::SIZE_X) continue;
            if (ny < 0 || ny >= Chunk::SIZE_Y) continue;
            if (nz < 0 || nz >= Chunk::SIZE_Z) continue;

            BlockType nb = chunk.getBlock(nx, ny, nz);
            int op = lightOpacity(nb);
            int newLight = (int)cur - 1 - op;
            if (newLight < 0) newLight = 0;

            if (newLight > chunk.getSkyLight(nx, ny, nz)) {
                chunk.setSkyLight(nx, ny, nz, (uint8_t)newLight);
                if (newLight > 1) q.push({ nx, ny, nz });
            }
        }
    }
}

void LightEngine::computeBlockLight(Chunk& chunk) {
    using Node = std::tuple<int, int, int>;
    std::queue<Node> q;

    for (int x = 0; x < Chunk::SIZE_X; x++) {
        for (int y = 0; y < Chunk::SIZE_Y; y++) {
            for (int z = 0; z < Chunk::SIZE_Z; z++) {
                uint8_t e = lightEmission(chunk.getBlock(x, y, z));
                if (e > 0) {
                    chunk.setBlockLight(x, y, z, e);
                    q.push({ x, y, z });
                }
            }
        }
    }

    static const int dx[6] = { 1,-1, 0, 0, 0, 0 };
    static const int dy[6] = { 0, 0, 1,-1, 0, 0 };
    static const int dz[6] = { 0, 0, 0, 0, 1,-1 };

    while (!q.empty()) {
        auto [cx, cy, cz] = q.front(); q.pop();
        uint8_t cur = chunk.getBlockLight(cx, cy, cz);
        if (cur <= 1) continue;

        for (int i = 0; i < 6; i++) {
            int nx = cx + dx[i], ny = cy + dy[i], nz = cz + dz[i];
            if (nx < 0 || nx >= Chunk::SIZE_X) continue;
            if (ny < 0 || ny >= Chunk::SIZE_Y) continue;
            if (nz < 0 || nz >= Chunk::SIZE_Z) continue;

            BlockType nb = chunk.getBlock(nx, ny, nz);
            int op = lightOpacity(nb);
            int newLight = (int)cur - 1 - op;
            if (newLight < 0) newLight = 0;

            if (newLight > chunk.getBlockLight(nx, ny, nz)) {
                chunk.setBlockLight(nx, ny, nz, (uint8_t)newLight);
                if (newLight > 1) q.push({ nx, ny, nz });
            }
        }
    }
}