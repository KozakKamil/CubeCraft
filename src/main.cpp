#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Texture.h"
#include "Shader.h"
#include "Camera.h"
#include "Chunk.h"

#include <iostream>
#include <vector>
#include <memory>

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

Camera camera;
float lastMouseX = WINDOW_WIDTH / 2.0f, lastMouseY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;

void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

void mouse_callback(GLFWwindow*, double x, double y) {
    if (firstMouse) { lastMouseX = (float)x; lastMouseY = (float)y; firstMouse = false; }
    float dx = (float)x - lastMouseX;
    float dy = lastMouseY - (float)y;
    lastMouseX = (float)x; lastMouseY = (float)y;
    camera.processMouse(dx, dy);
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
        "CubeCraft - Chunks", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    std::cout << "GPU: " << glGetString(GL_RENDERER) << "\n";

    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);  
    //glCullFace(GL_BACK);
    //glFrontFace(GL_CW);     

    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Texture grassAtlas;

    
    std::vector<std::unique_ptr<Chunk>> chunks;
    for (int cx = -1; cx <= 1; cx++) {
        for (int cz = -1; cz <= 1; cz++) {
            auto c = std::make_unique<Chunk>(glm::ivec3(cx, 0, cz));
            c->generateTerrain();
            c->buildMesh();
            chunks.push_back(std::move(c));
        }
    }

    shader.use();
    shader.setInt("uTexture", 0);

    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float t = (float)glfwGetTime();
        float dt = t - lastFrame;
        lastFrame = t;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        camera.processKeyboard(window, dt);

        glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        grassAtlas.bind(0);

        glm::mat4 view = camera.viewMatrix();
        glm::mat4 proj = glm::perspective(glm::radians(70.0f),
            (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 1000.0f);

        shader.setMat4("uView", view);
        shader.setMat4("uProjection", proj);

        
        for (const auto& c : chunks) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f), c->worldPos());
            shader.setMat4("uModel", model);
            c->draw();
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}