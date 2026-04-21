#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

#include "Texture.h"
#include "Shader.h"
#include "Camera.h"
#include "World.h"

#include <iostream>

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
        "CubeCraft - Perlin World", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

    glEnable(GL_DEPTH_TEST);

    Shader shader("shaders/basic.vert", "shaders/basic.frag");
    Texture grassAtlas;

    World world(1337);
    world.generateInitial(5);  

    camera.position = glm::vec3(0.0f, 30.0f, 0.0f);
    camera.pitch = -30.0f;
    camera.processMouse(0, 0);

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

        world.draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}