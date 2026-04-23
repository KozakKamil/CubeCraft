#include "Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <iostream>
#include <vector>
#include <cstdint>

Texture::Texture(const std::string& path) {
    stbi_set_flip_vertically_on_load(true);

    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Nie mozna zaladowac tekstury: " << path << "\n";
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
    std::cout << "Zaladowano teksture: " << path << " (" << width << "x" << height << ")\n";
}
Texture::Texture() {
    constexpr int TILE = 16;
    constexpr int TILES = 9;
    constexpr int W = TILE * TILES;  
    constexpr int H = TILE;          
    std::vector<uint8_t> pixels(W * H * 3);

    auto setPx = [&](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
        int idx = (y * W + x) * 3;
        pixels[idx + 0] = r;
        pixels[idx + 1] = g;
        pixels[idx + 2] = b;
        };

    auto noise = [](int x, int y) { return ((x * 73 + y * 19) % 13) - 6; };
    auto stoneNoise = [](int x, int y) { return ((x * 127 + y * 31 + x * y) % 25) - 12; };
    auto sandNoise = [](int x, int y) { return ((x * 53 + y * 91) % 9) - 4; };
    auto snowNoise = [](int x, int y) { return ((x * 211 + y * 47) % 15) - 7; };
    auto waterNoise = [](int x, int y) { return ((x * 113 + y * 67) % 19) - 9; };
	auto woodNoise = [](int x, int y) { return ((x * 97 + y * 41) % 11) - 5; };
	auto leavesNoise = [](int x, int y) { return ((x * 151 + y * 59) % 17) - 8; };

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < TILE; x++) {
            int n = noise(x, y);

            setPx(x + TILE*0, y, 90 + n, 150 + n, 60 + n);

            int sideX = x + TILE*1;
            if (y >= H - 3) setPx(sideX, y, 90 + n, 150 + n, 60 + n);
            else            setPx(sideX, y, 134 + n, 96 + n, 67 + n);

            setPx(x + TILE * 2, y, 134 + n, 96 + n, 67 + n);

            int sn = stoneNoise(x, y);
            setPx(x + TILE * 3, y, 125 + sn, 125 + sn, 125 + sn);

            int san = sandNoise(x, y);
            setPx(x + TILE * 4, y, 220 + san, 200 + san, 140 + san);

            int snw = snowNoise(x, y);
            setPx(x + TILE * 5, y, 235 + snw, 240 + snw, 245 + snw);

            int wn = waterNoise(x, y);
            setPx(x + TILE * 6, y, 40 + wn, 90 + wn, 170 + wn);

			int wdn = woodNoise(x, y);
			setPx(x + TILE * 7, y, 120 + wdn, 80 + wdn, 50 + wdn);

			int ln = leavesNoise(x, y);
			setPx(x + TILE * 8, y, 60 + ln, 120 + ln, 55 + ln);
        }
    }

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    std::cout << "Wygenerowano atlas proceduralny " << W << "x" << H << " (" << TILES << " tiles)\n";
}

Texture::~Texture() {
    if (m_id) glDeleteTextures(1, &m_id);
}

void Texture::bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}