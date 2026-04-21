#include "Chunk.h"
#include <iostream>

static constexpr float U0 = 0.0f, U1 = 1.0f / 4.0f;
static constexpr float U2 = 1.0f / 4.0f, U3 = 2.0f / 4.0f;
static constexpr float U4 = 2.0f / 4.0f, U5 = 3.0f / 4.0f;
static constexpr float U6 = 3.0f / 4.0f, U7 = 1.0f;

static void getUVRange(BlockType type, int face, float& uMin, float& uMax) {
	if (type == BlockType::Grass) {
		if (face == 2) { uMin = U0; uMax = U1; }
		else if (face == 3) { uMin = U4; uMax = U5; }
		else { uMin = U2; uMax = U3; }
	}
	else if (type == BlockType::Dirt) {
		uMin = U4; uMax = U5;
	}
	else if (type == BlockType::Stone) {
		uMin = U6; uMax = U7;
	}
	else {
		uMin = 0; uMax = 0;
	}
}

Chunk::Chunk(glm::ivec3 pos) : m_pos(pos) {
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);
}

Chunk::~Chunk() {
	if (m_VBO) glDeleteBuffers(1, &m_VBO);
	if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
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
    std::vector<float> verts;
    verts.reserve(10000);

    auto addFace = [&](int x, int y, int z, int face, BlockType type) {
        float uMin, uMax;
        getUVRange(type, face, uMin, uMax);

        float fx = (float)x, fy = (float)y, fz = (float)z;

        // Normal w zaleznosci od kierunku sciany
        float nx = 0, ny = 0, nz = 0;
        switch (face) {
        case 0: nx = 1; break; // +X
        case 1: nx = -1; break; // -X
        case 2: ny = 1; break; // +Y
        case 3: ny = -1; break; // -Y
        case 4: nz = 1; break; // +Z
        case 5: nz = -1; break; // -Z
        }

        // Helper do wpisywania jednego wierzcholka (8 floatow)
        auto push = [&](float px, float py, float pz, float u, float v) {
            verts.push_back(px);
            verts.push_back(py);
            verts.push_back(pz);
            verts.push_back(u);
            verts.push_back(v);
            verts.push_back(nx);
            verts.push_back(ny);
            verts.push_back(nz);
            };

        switch (face) {
        case 0: // +X (prawo)
            push(fx + 1, fy, fz, uMin, 0);
            push(fx + 1, fy, fz + 1, uMax, 0);
            push(fx + 1, fy + 1, fz + 1, uMax, 1);
            push(fx + 1, fy + 1, fz + 1, uMax, 1);
            push(fx + 1, fy + 1, fz, uMin, 1);
            push(fx + 1, fy, fz, uMin, 0);
            break;
        case 1: // -X (lewo)
            push(fx, fy, fz + 1, uMin, 0);
            push(fx, fy, fz, uMax, 0);
            push(fx, fy + 1, fz, uMax, 1);
            push(fx, fy + 1, fz, uMax, 1);
            push(fx, fy + 1, fz + 1, uMin, 1);
            push(fx, fy, fz + 1, uMin, 0);
            break;
        case 2: // +Y (gora)
            push(fx, fy + 1, fz, uMin, 0);
            push(fx + 1, fy + 1, fz, uMax, 0);
            push(fx + 1, fy + 1, fz + 1, uMax, 1);
            push(fx + 1, fy + 1, fz + 1, uMax, 1);
            push(fx, fy + 1, fz + 1, uMin, 1);
            push(fx, fy + 1, fz, uMin, 0);
            break;
        case 3: // -Y (dol)
            push(fx, fy, fz + 1, uMin, 0);
            push(fx + 1, fy, fz + 1, uMax, 0);
            push(fx + 1, fy, fz, uMax, 1);
            push(fx + 1, fy, fz, uMax, 1);
            push(fx, fy, fz, uMin, 1);
            push(fx, fy, fz + 1, uMin, 0);
            break;
        case 4: // +Z (przod)
            push(fx, fy, fz + 1, uMin, 0);
            push(fx + 1, fy, fz + 1, uMax, 0);
            push(fx + 1, fy + 1, fz + 1, uMax, 1);
            push(fx + 1, fy + 1, fz + 1, uMax, 1);
            push(fx, fy + 1, fz + 1, uMin, 1);
            push(fx, fy, fz + 1, uMin, 0);
            break;
        case 5: // -Z (tyl)
            push(fx + 1, fy, fz, uMin, 0);
            push(fx, fy, fz, uMax, 0);
            push(fx, fy + 1, fz, uMax, 1);
            push(fx, fy + 1, fz, uMax, 1);
            push(fx + 1, fy + 1, fz, uMin, 1);
            push(fx + 1, fy, fz, uMin, 0);
            break;
        }
        };

    // Petla po blokach (bez zmian)
    for (int x = 0; x < SIZE_X; x++) {
        for (int y = 0; y < SIZE_Y; y++) {
            for (int z = 0; z < SIZE_Z; z++) {
                BlockType cur = m_bloks[x][y][z];
                if (isAir(cur)) continue;

                if (isAir(getBlock(x + 1, y, z))) addFace(x, y, z, 0, cur);
                if (isAir(getBlock(x - 1, y, z))) addFace(x, y, z, 1, cur);
                if (isAir(getBlock(x, y + 1, z))) addFace(x, y, z, 2, cur);
                if (isAir(getBlock(x, y - 1, z))) addFace(x, y, z, 3, cur);
                if (isAir(getBlock(x, y, z + 1))) addFace(x, y, z, 4, cur);
                if (isAir(getBlock(x, y, z - 1))) addFace(x, y, z, 5, cur);
            }
        }
    }

    // Upload na GPU - zmienil sie STRIDE (8 floatow zamiast 5)
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    constexpr GLsizei STRIDE = 8 * sizeof(float);

    // Atrybut 0: pozycja (3 floaty, offset 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)0);
    glEnableVertexAttribArray(0);
    // Atrybut 1: UV (2 floaty, offset 12)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, STRIDE, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // Atrybut 2: normal (3 floaty, offset 20)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, STRIDE, (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    m_vertexCount = (GLsizei)(verts.size() / 8);

    std::cout << "Chunk (" << m_pos.x << "," << m_pos.y << "," << m_pos.z
        << ") mesh: " << m_vertexCount << " vertexow\n";
}

void Chunk::draw() const {
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
}