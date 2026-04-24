#include "Chunk.h"
#include "World.h"
#include <iostream>
#include <glm/glm.hpp>

static constexpr float TILE_U = 1.0f / 9.0f;

static void getUVRange(BlockType type, int face, float& uMin, float& uMax) {
    int tile = 0;
    if (type == BlockType::Grass) {
        if (face == 2)      tile = 0;
        else if (face == 3) tile = 2; 
        else                tile = 1;
    }
    else if (type == BlockType::Dirt)  tile = 2;
    else if (type == BlockType::Stone) tile = 3;
    else if (type == BlockType::Sand)  tile = 4;
    else if (type == BlockType::Snow)  tile = 5;
    else if (type == BlockType::Water) tile = 6;
    else if (type == BlockType::Wood)  tile = 7;
	else if (type == BlockType::Leaves) tile = 8;
    else { uMin = 0; uMax = 0; return; }

    uMin = tile * TILE_U;
    uMax = (tile + 1) * TILE_U;
}

static float computeAOCorner(bool side1, bool side2, bool cornerBlock) {
	if (side1 && side2) return 0.25f;
	int occluded = (side1 ? 1 : 0) + (side2 ? 1 : 0) + (cornerBlock ? 1 : 0);
	return 1.0f - 0.25f * (float)occluded;
}

static bool aoSolid(BlockType b) {
	return !(b == BlockType::Air || b == BlockType::Water);
}


static bool shouldDrawFace(BlockType current, BlockType neighbor) {
    if (isAir(neighbor)) return true;
    if (current == BlockType::Water) return false;       
    if (neighbor == BlockType::Water) return true;       
    return false;
}

Chunk::Chunk(glm::ivec3 pos) : m_pos(pos) {

}

Chunk::~Chunk() {
	if (m_VBO) glDeleteBuffers(1, &m_VBO);
	if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
}

void Chunk::initGL() {
    if (m_glInitialized) return;
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    m_glInitialized = true;
}

bool Chunk::inBounds(int x, int y, int z) const {
	return x >= 0 && x < SIZE_X && y >= 0 && y < SIZE_Y && z >= 0 && z < SIZE_Z;
}

BlockType Chunk::getBlock(int x, int y, int z) const {
	if (!inBounds(x, y, z)) return BlockType::Air;
	return m_bloks[x][y][z];
}

void Chunk::setBlock(int x, int y, int z, BlockType t) {
	if (inBounds(x, y, z)) m_bloks[x][y][z] = t;
}

BlockType Chunk::getBlockForCulling(int x, int y, int z) const {
	if (y < 0 || y >= SIZE_Y) return BlockType::Air;

    if(x >= 0 && x < SIZE_X && z >= 0 && z < SIZE_Z) {
        return m_bloks[x][y][z];
    }

	if (!m_world) return BlockType::Air;

	int wx = m_pos.x * SIZE_X + x;
	int wz = m_pos.z * SIZE_Z + z;

	return m_world->getBlockWorld(wx, y, wz);
}

void Chunk::generateTerrain() {
	const int grassY = 12;
	for (int x = 0; x < SIZE_X; x++) {
		for (int z = 0; z < SIZE_Z; z++) {
			for (int y = 0; y < SIZE_Y; y++) {
				if (y < grassY - 3) m_bloks[x][y][z] = BlockType::Stone;
				else if (y < grassY) m_bloks[x][y][z] = BlockType::Dirt;
				else if (y == grassY) m_bloks[x][y][z] = BlockType::Grass;
				else m_bloks[x][y][z] = BlockType::Air;
			}
		}
	}
}

void Chunk::buildMesh() {
    initGL();
    std::vector<float> verts;
    verts.reserve(20000);

    // helper -- szybki lookup z uwzglednieniem chunkow sasiadow
    auto neighborSolid = [&](int x, int y, int z) -> bool {
        return aoSolid(getBlockForCulling(x, y, z));
        };

    auto addFace = [&](int x, int y, int z, int face, BlockType type) {
        float uMin, uMax;
        getUVRange(type, face, uMin, uMax);

        float fx = (float)x, fy = (float)y, fz = (float)z;

        float nx = 0, ny = 0, nz = 0;
        switch (face) {
        case 0: nx = 1;  break;
        case 1: nx = -1; break;
        case 2: ny = 1;  break;
        case 3: ny = -1; break;
        case 4: nz = 1;  break;
        case 5: nz = -1; break;
        }

        float ao[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

        int ox = x + (int)nx, oy = y + (int)ny, oz = z + (int)nz;

        auto aoAt = [&](int du1, int dv1, int du2, int dv2) {

            auto S = [&](int du, int dv) {

                int dx = 0, dy = 0, dz = 0;
                switch (face) {
                case 0: case 1: dy = du; dz = dv; break;
                case 2: case 3: dx = du; dz = dv; break;
                case 4: case 5: dx = du; dy = dv; break;
                }
                return neighborSolid(ox + dx, oy + dy, oz + dz);
                };
            bool s1 = S(du1, dv1);
            bool s2 = S(du2, dv2);
            bool c = S(du1 + du2, dv1 + dv2);
            return computeAOCorner(s1, s2, c);
            };

        ao[0] = aoAt(-1, 0, 0, -1); 
        ao[1] = aoAt(1, 0, 0, -1); 
        ao[2] = aoAt(1, 0, 0, 1); 
        ao[3] = aoAt(-1, 0, 0, 1); 

        auto push = [&](float px, float py, float pz, float u, float v, float aoVal) {
            verts.push_back(px); verts.push_back(py); verts.push_back(pz);
            verts.push_back(u);  verts.push_back(v);
            verts.push_back(nx); verts.push_back(ny); verts.push_back(nz);
            verts.push_back(aoVal);
            };

        bool flip = (ao[0] + ao[2] < ao[1] + ao[3]);

        glm::vec3 p[4];
        switch (face) {
        case 0:
            p[0] = { fx + 1, fy,     fz };
            p[1] = { fx + 1, fy,     fz + 1 };
            p[2] = { fx + 1, fy + 1, fz + 1 };
            p[3] = { fx + 1, fy + 1, fz };
            break;
        case 1:
            p[0] = { fx, fy,     fz + 1 };
            p[1] = { fx, fy,     fz };
            p[2] = { fx, fy + 1, fz };
            p[3] = { fx, fy + 1, fz + 1 };
            break;
        case 2:
            p[0] = { fx,     fy + 1, fz };
            p[1] = { fx + 1, fy + 1, fz };
            p[2] = { fx + 1, fy + 1, fz + 1 };
            p[3] = { fx,     fy + 1, fz + 1 };
            break;
        case 3:
            p[0] = { fx,     fy, fz + 1 };
            p[1] = { fx + 1, fy, fz + 1 };
            p[2] = { fx + 1, fy, fz };
            p[3] = { fx,     fy, fz };
            break;
        case 4:
            p[0] = { fx,     fy,     fz + 1 };
            p[1] = { fx + 1, fy,     fz + 1 };
            p[2] = { fx + 1, fy + 1, fz + 1 };
            p[3] = { fx,     fy + 1, fz + 1 };
            break;
        case 5:
            p[0] = { fx + 1, fy,     fz };
            p[1] = { fx,     fy,     fz };
            p[2] = { fx,     fy + 1, fz };
            p[3] = { fx + 1, fy + 1, fz };
            break;
        }

        float cu[4] = { uMin, uMax, uMax, uMin };
        float cv[4] = { 0,    0,    1,    1 };

        int idx[6];
        if (!flip) { idx[0] = 0; idx[1] = 1; idx[2] = 2; idx[3] = 2; idx[4] = 3; idx[5] = 0; }
        else { idx[0] = 0; idx[1] = 1; idx[2] = 3; idx[3] = 3; idx[4] = 1; idx[5] = 2; }

        for (int k = 0; k < 6; k++) {
            int i = idx[k];
            push(p[i].x, p[i].y, p[i].z, cu[i], cv[i], ao[i]);
        }
        };

    for (int x = 0; x < SIZE_X; x++) {
        for (int y = 0; y < SIZE_Y; y++) {
            for (int z = 0; z < SIZE_Z; z++) {
                BlockType cur = m_bloks[x][y][z];
                if (isAir(cur)) continue;

                if (shouldDrawFace(cur, getBlockForCulling(x + 1, y, z))) addFace(x, y, z, 0, cur);
                if (shouldDrawFace(cur, getBlockForCulling(x - 1, y, z))) addFace(x, y, z, 1, cur);
                if (shouldDrawFace(cur, getBlockForCulling(x, y + 1, z))) addFace(x, y, z, 2, cur);
                if (shouldDrawFace(cur, getBlockForCulling(x, y - 1, z))) addFace(x, y, z, 3, cur);
                if (shouldDrawFace(cur, getBlockForCulling(x, y, z + 1))) addFace(x, y, z, 4, cur);
                if (shouldDrawFace(cur, getBlockForCulling(x, y, z - 1))) addFace(x, y, z, 5, cur);
            }
        }
    }

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    constexpr GLsizei STRIDE = 9 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, STRIDE, (void*)(8 * sizeof(float)));  
    glEnableVertexAttribArray(3);                                                          

    m_vertexCount = (GLsizei)(verts.size() / 9);
}

void Chunk::draw() const {
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
}