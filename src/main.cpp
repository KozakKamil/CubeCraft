#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Texture.h"
#include "Shader.h"
#include "Camera.h"
#include "World.h"
#include "Skybox.h"

#include <iostream>

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

Camera camera;
float lastMouseX = WINDOW_WIDTH / 2.0f, lastMouseY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;

World* g_world = nullptr;
BlockType currentBlock = BlockType::Grass;

void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

void mouse_callback(GLFWwindow*, double x, double y) {
    if (firstMouse) { lastMouseX = (float)x; lastMouseY = (float)y; firstMouse = false; }
    float dx = (float)x - lastMouseX;
    float dy = lastMouseY - (float)y;
    lastMouseX = (float)x; lastMouseY = (float)y;
    camera.processMouse(dx, dy);
}

void mouse_button_callback(GLFWwindow* win, int button, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (!g_world) return;

    RaycastHit hit = g_world->raycast(camera.position, camera.front, 6.0f);
    if (!hit.hit) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        g_world->setBlockWorld(hit.blockPos.x, hit.blockPos.y, hit.blockPos.z, BlockType::Air);
        std::cout << "Zniszczony blok na (" << hit.blockPos.x << ","
            << hit.blockPos.y << "," << hit.blockPos.z << ")\n";
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        g_world->setBlockWorld(hit.prevPos.x, hit.prevPos.y, hit.prevPos.z, currentBlock);
        std::cout << "Postawiony blok na (" << hit.prevPos.x << ","
            << hit.prevPos.y << "," << hit.prevPos.z << ")\n";
    }
}

void key_callback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_1) { currentBlock = BlockType::Grass; std::cout << "Wybrano: Trawa\n"; }
    if (key == GLFW_KEY_2) { currentBlock = BlockType::Dirt;  std::cout << "Wybrano: Ziemia\n"; }
    if (key == GLFW_KEY_3) { currentBlock = BlockType::Stone; std::cout << "Wybrano: Kamien\n"; }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
        "CubeCraft - Interactive", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "\n=== STEROWANIE ===\n";
    std::cout << "WSAD + Spacja/Ctrl - ruch\n";
    std::cout << "Mysz - rozgladanie\n";
    std::cout << "LPM - niszcz blok\n";
    std::cout << "PPM - postaw blok\n";
    std::cout << "1/2/3 - wybor bloku (trawa/ziemia/kamien)\n";
    std::cout << "ESC - wyjscie\n\n";

    glEnable(GL_DEPTH_TEST);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Texture grassAtlas;

    World world(1337);
    Skybox skybox;
    world.generateInitial(5);
    g_world = &world;

    camera.position = glm::vec3(0.0f, 30.0f, 0.0f);
    camera.pitch = -30.0f;
    camera.processMouse(0, 0);

    shader.setInt("uTexture", 0);

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float t = (float)glfwGetTime();
        float dt = t - lastFrame;
        lastFrame = t;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        camera.processKeyboard(window, dt);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



        glm::mat4 view = camera.viewMatrix();
        glm::mat4 proj = glm::perspective(glm::radians(70.0f),
            (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 1000.0f);

        glm::vec3 lightDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.4f));
        glm::vec3 sunDir = -lightDir;
        skybox.draw(view, proj, sunDir);

        shader.use();
        grassAtlas.bind(0);
        shader.setMat4("uView", view);
        shader.setMat4("uProjection", proj);
        shader.setVec3("uLightDir", lightDir);
        shader.setFloat("uAmbient", 0.35f);

        shader.setVec3("uCameraPos", camera.position);
        shader.setVec3("uFogColor", glm::vec3(0.75f, 0.85f, 0.95f));
        shader.setFloat("uFogStart", 60.0f);
        shader.setFloat("uFogEnd", 110.0f);

        world.draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}