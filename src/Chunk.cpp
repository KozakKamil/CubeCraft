#include "Chunk.h"
#include <iostream>

static constexpr float U0 = 0.0f, U1 = 1.0f / 3.0f;
static constexpr float U2 = 1.0f / 3.0f, U3 = 2.0f / 3.0f;
static constexpr float U4 = 2.0f / 3.0f, U5 = 1.0f;

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
		uMin = U4; uMax = U5;
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

        float quad[6 * 5];

        switch (face) {
        case 0:
            quad[0] = fx + 1; quad[1] = fy;   quad[2] = fz;   quad[3] = uMin; quad[4] = 0;
            quad[5] = fx + 1; quad[6] = fy;   quad[7] = fz + 1; quad[8] = uMax; quad[9] = 0;
            quad[10] = fx + 1; quad[11] = fy + 1; quad[12] = fz + 1; quad[13] = uMax; quad[14] = 1;
            quad[15] = fx + 1; quad[16] = fy + 1; quad[17] = fz + 1; quad[18] = uMax; quad[19] = 1;
            quad[20] = fx + 1; quad[21] = fy + 1; quad[22] = fz;  quad[23] = uMin; quad[24] = 1;
            quad[25] = fx + 1; quad[26] = fy;  quad[27] = fz;  quad[28] = uMin; quad[29] = 0;
            break;
        case 1:
            quad[0] = fx;   quad[1] = fy;   quad[2] = fz + 1; quad[3] = uMin; quad[4] = 0;
            quad[5] = fx;   quad[6] = fy;   quad[7] = fz;   quad[8] = uMax; quad[9] = 0;
            quad[10] = fx;  quad[11] = fy + 1; quad[12] = fz;  quad[13] = uMax; quad[14] = 1;
            quad[15] = fx;  quad[16] = fy + 1; quad[17] = fz;  quad[18] = uMax; quad[19] = 1;
            quad[20] = fx;  quad[21] = fy + 1; quad[22] = fz + 1; quad[23] = uMin; quad[24] = 1;
            quad[25] = fx;  quad[26] = fy;  quad[27] = fz + 1; quad[28] = uMin; quad[29] = 0;
            break;
        case 2:
            quad[0] = fx;   quad[1] = fy + 1; quad[2] = fz;   quad[3] = uMin; quad[4] = 0;
            quad[5] = fx + 1; quad[6] = fy + 1; quad[7] = fz;   quad[8] = uMax; quad[9] = 0;
            quad[10] = fx + 1; quad[11] = fy + 1; quad[12] = fz + 1; quad[13] = uMax; quad[14] = 1;
            quad[15] = fx + 1; quad[16] = fy + 1; quad[17] = fz + 1; quad[18] = uMax; quad[19] = 1;
            quad[20] = fx;  quad[21] = fy + 1; quad[22] = fz + 1; quad[23] = uMin; quad[24] = 1;
            quad[25] = fx;  quad[26] = fy + 1; quad[27] = fz;  quad[28] = uMin; quad[29] = 0;
            break;
        case 3:
            quad[0] = fx;   quad[1] = fy;   quad[2] = fz + 1; quad[3] = uMin; quad[4] = 0;
            quad[5] = fx + 1; quad[6] = fy;   quad[7] = fz + 1; quad[8] = uMax; quad[9] = 0;
            quad[10] = fx + 1; quad[11] = fy;  quad[12] = fz;  quad[13] = uMax; quad[14] = 1;
            quad[15] = fx + 1; quad[16] = fy;  quad[17] = fz;  quad[18] = uMax; quad[19] = 1;
            quad[20] = fx;  quad[21] = fy;  quad[22] = fz;  quad[23] = uMin; quad[24] = 1;
            quad[25] = fx;  quad[26] = fy;  quad[27] = fz + 1; quad[28] = uMin; quad[29] = 0;
            break;
        case 4:
            quad[0] = fx;   quad[1] = fy;   quad[2] = fz + 1; quad[3] = uMin; quad[4] = 0;
            quad[5] = fx + 1; quad[6] = fy;   quad[7] = fz + 1; quad[8] = uMax; quad[9] = 0;
            quad[10] = fx + 1; quad[11] = fy + 1; quad[12] = fz + 1; quad[13] = uMax; quad[14] = 1;
            quad[15] = fx + 1; quad[16] = fy + 1; quad[17] = fz + 1; quad[18] = uMax; quad[19] = 1;
            quad[20] = fx;  quad[21] = fy + 1; quad[22] = fz + 1; quad[23] = uMin; quad[24] = 1;
            quad[25] = fx;  quad[26] = fy;  quad[27] = fz + 1; quad[28] = uMin; quad[29] = 0;
            break;
        case 5:
            quad[0] = fx + 1; quad[1] = fy;   quad[2] = fz;   quad[3] = uMin; quad[4] = 0;
            quad[5] = fx;   quad[6] = fy;   quad[7] = fz;   quad[8] = uMax; quad[9] = 0;
            quad[10] = fx;  quad[11] = fy + 1; quad[12] = fz;  quad[13] = uMax; quad[14] = 1;
            quad[15] = fx;  quad[16] = fy + 1; quad[17] = fz;  quad[18] = uMax; quad[19] = 1;
            quad[20] = fx + 1; quad[21] = fy + 1; quad[22] = fz;  quad[23] = uMin; quad[24] = 1;
            quad[25] = fx + 1; quad[26] = fy;  quad[27] = fz;  quad[28] = uMin; quad[29] = 0;
            break;
        }
        verts.insert(verts.end(), std::begin(quad), std::end(quad));
     };

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

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_vertexCount = (GLsizei)(verts.size() / 5);

    std::cout << "Chunk (" << m_pos.x << "," << m_pos.y << "," << m_pos.z << ") mesh: " << m_vertexCount << "  vertexow\n";
}

void Chunk::draw() const {
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_TRIANGLES, 0, m_vertexCount);
}