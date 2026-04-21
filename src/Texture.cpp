#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <iostream>
#include <vector>
#include <cstdint>

// Loading from file
Texture::Texture(const std::string& path) {
	stbi_set_flip_vertically_on_load(true);

	int width, height, channels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
	if (!data) {
		std::cerr << "Cannot load texture: " << path << "\n";
		return;
	}

	GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;

	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);
	std::cout << "Texture loaded: " << path << " (" << width << "x" << height << ")\n";

}

//Procedulal atla 48x16 (3 fields 16x16 next to each other
// Field 0: Top
// Field 1: Side 
// Field 2: Bottom


Texture::Texture() {
	constexpr int TILE = 16;
	constexpr int W = TILE * 3;
	constexpr int H = TILE;
	std::vector<uint8_t> pixels(W * H * 3);

	auto setPixel = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
		int idx = (y * W + x) * 3;
		pixels[idx + 0] = r;
		pixels[idx + 1] = g;
		pixels[idx + 2] = b;
	};

	auto noise = [](int x, int y) -> int {
		return((x * 73 + y * 19) % 13) - 6;
	};

	for (int y = 0; y < H; y++) {
		for (int x = 0; x < TILE; x++) {
			int n = noise(x, y);
			setPixel(x, y, 90 + n, 150 + n, 60 + n);
			int sideX = x + TILE;
			if (y >= H - 3) {
				setPixel(sideX, y, 90 + n, 150 + n, 60 + n);
			}
			else {
				setPixel(sideX, y, 134 + n, 96 + n, 67 + n);
			}

			int botX = x + TILE * 2;
			setPixel(botX, y, 134 + n, 96 + n, 67 + n);
		}
	}

	glGenTextures(1, &m_id);
	glBindTexture(GL_TEXTURE_2D, m_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
	std::cout << "Gnerated procedural atlas " << W << "x" << H << "\n";
}

Texture::~Texture() {
	if (m_id) glDeleteTextures(1, &m_id);
}

void Texture::bind(GLuint unit) const {
	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, m_id);
}