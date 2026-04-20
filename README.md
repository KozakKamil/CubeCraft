# CubeCraft 🧱

Voxel sandbox game inspired by classic Minecraft Alpha. Written from scratch in C++ with OpenGL.

## Stack
- **Language:** C++20
- **Build:** CMake + vcpkg
- **Graphics:** OpenGL 3.3 Core
- **Libraries:** GLFW (window/input), GLAD (GL loader), GLM (math), stb_image (textures)

## Progress

### Etap 0: OpenGL Bootcamp
- [x] Puste okno OpenGL (zielone tło)
- [ ] Pierwszy trójkąt (VAO/VBO/shadery)
- [ ] Tekstury
- [ ] Kamera FPP (WSAD + mysz)
- [ ] Oświetlenie podstawowe

### Etap 1: Pierwszy chunk
- [ ] Klasa Block i Chunk
- [ ] Mesh generation
- [ ] Face culling
- [ ] Texture atlas
- [ ] Render wielu chunków

### Etap 2: Generacja świata
- [ ] Perlin noise heightmap
- [ ] Biomy
- [ ] Drzewa i jaskinie
- [ ] Seed świata

_(dalsze etapy w roadmapie)_

## Build

```powershell
# Wymaga vcpkg zintegrowanego z VS (vcpkg integrate install)
git clone https://github.com/KozakKamil/CubeCraft.git
cd CubeCraft
# Otwórz folder w Visual Studio 2022 (File → Open → Folder)
# VS sam wykryje CMakeLists.txt i zbuduje projekt
```

## Sterowanie
- `ESC` — wyjście