#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Texture.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

// === Kamera ===
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f, pitch = 0.0f;
float lastMouseX = WINDOW_WIDTH / 2.0f, lastMouseY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

std::string loadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) { std::cerr << "Cannot read: " << path << "\n"; return ""; }
    std::stringstream ss; ss << file.rdbuf(); return ss.str();
}

GLuint compileShader(GLenum type, const std::string& src) {
    GLuint s = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(s, 1, &c, nullptr);
    glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s, 512, nullptr, log); std::cerr << "Shader err:\n" << log << "\n"; }
    return s;
}

GLuint createShaderProgram(const std::string& vp, const std::string& fp) {
    GLuint v = compileShader(GL_VERTEX_SHADER, loadFile(vp));
    GLuint f = compileShader(GL_FRAGMENT_SHADER, loadFile(fp));
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    GLint ok; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char log[512]; glGetProgramInfoLog(p, 512, nullptr, log); std::cerr << "Link err:\n" << log << "\n"; }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

void framebuffer_size_callback(GLFWwindow*, int w, int h) { glViewport(0, 0, w, h); }

void mouse_callback(GLFWwindow*, double x, double y) {
    if (firstMouse) { lastMouseX = (float)x; lastMouseY = (float)y; firstMouse = false; }
    float dx = (float)x - lastMouseX;
    float dy = lastMouseY - (float)y;
    lastMouseX = (float)x; lastMouseY = (float)y;
    float sens = 0.1f;
    yaw += dx * sens; pitch += dy * sens;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 f;
    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    f.y = sin(glm::radians(pitch));
    f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(f);
}

void processInput(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, true);
    float sp = 5.0f * deltaTime;
    if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) sp *= 3.0f;
    if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) cameraPos += sp * cameraFront;
    if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= sp * cameraFront;
    if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * sp;
    if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * sp;
    if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos += sp * cameraUp;
    if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) cameraPos -= sp * cameraUp;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "CubeCraft - Textured", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";

    glEnable(GL_DEPTH_TEST);

    // ============================================================
    // Kostka z UV - atlas 48x16 z 3 polami (0-1/3, 1/3-2/3, 2/3-1)
    //  TILE 0 (0.00 - 0.33) = gora (trawa)
    //  TILE 1 (0.33 - 0.67) = bok (ziemia+trawa)
    //  TILE 2 (0.67 - 1.00) = dol (ziemia)
    // ============================================================
    constexpr float U0 = 0.0f, U1 = 1.0f / 3.0f;  // tile 0
    constexpr float U2 = 1.0f / 3.0f, U3 = 2.0f / 3.0f; // tile 1
    constexpr float U4 = 2.0f / 3.0f, U5 = 1.0f;        // tile 2

    float vertices[] = {
        // pozycja              // UV
        // Tyl (bok)
        -0.5f, -0.5f, -0.5f,    U3, 0.0f,
         0.5f, -0.5f, -0.5f,    U2, 0.0f,
         0.5f,  0.5f, -0.5f,    U2, 1.0f,
         0.5f,  0.5f, -0.5f,    U2, 1.0f,
        -0.5f,  0.5f, -0.5f,    U3, 1.0f,
        -0.5f, -0.5f, -0.5f,    U3, 0.0f,

        // Przod (bok)
        -0.5f, -0.5f,  0.5f,    U2, 0.0f,
         0.5f, -0.5f,  0.5f,    U3, 0.0f,
         0.5f,  0.5f,  0.5f,    U3, 1.0f,
         0.5f,  0.5f,  0.5f,    U3, 1.0f,
        -0.5f,  0.5f,  0.5f,    U2, 1.0f,
        -0.5f, -0.5f,  0.5f,    U2, 0.0f,

        // Lewo (bok)
        -0.5f,  0.5f,  0.5f,    U3, 1.0f,
        -0.5f,  0.5f, -0.5f,    U2, 1.0f,
        -0.5f, -0.5f, -0.5f,    U2, 0.0f,
        -0.5f, -0.5f, -0.5f,    U2, 0.0f,
        -0.5f, -0.5f,  0.5f,    U3, 0.0f,
        -0.5f,  0.5f,  0.5f,    U3, 1.0f,

        // Prawo (bok)
         0.5f,  0.5f,  0.5f,    U2, 1.0f,
         0.5f,  0.5f, -0.5f,    U3, 1.0f,
         0.5f, -0.5f, -0.5f,    U3, 0.0f,
         0.5f, -0.5f, -0.5f,    U3, 0.0f,
         0.5f, -0.5f,  0.5f,    U2, 0.0f,
         0.5f,  0.5f,  0.5f,    U2, 1.0f,

         // Dol (ziemia)
         -0.5f, -0.5f, -0.5f,    U4, 0.0f,
          0.5f, -0.5f, -0.5f,    U5, 0.0f,
          0.5f, -0.5f,  0.5f,    U5, 1.0f,
          0.5f, -0.5f,  0.5f,    U5, 1.0f,
         -0.5f, -0.5f,  0.5f,    U4, 1.0f,
         -0.5f, -0.5f, -0.5f,    U4, 0.0f,

         // Gora (trawa)
         -0.5f,  0.5f, -0.5f,    U0, 0.0f,
          0.5f,  0.5f, -0.5f,    U1, 0.0f,
          0.5f,  0.5f,  0.5f,    U1, 1.0f,
          0.5f,  0.5f,  0.5f,    U1, 1.0f,
         -0.5f,  0.5f,  0.5f,    U0, 1.0f,
         -0.5f,  0.5f, -0.5f,    U0, 0.0f
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Atrybut 0: pozycja (3 floaty)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Atrybut 1: UV (2 floaty, offset 12 bajtow)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint shaderProgram = createShaderProgram("shaders/basic.vert", "shaders/basic.frag");

    // Proceduralny atlas trawy
    Texture grassAtlas;

    glm::vec3 cubePositions[] = {
        glm::vec3(0.0f,  0.0f,  0.0f),
        glm::vec3(2.0f,  0.0f, -3.0f),
        glm::vec3(-2.5f,  1.0f, -5.0f),
        glm::vec3(1.5f, -1.0f, -7.0f),
        glm::vec3(-1.0f,  0.5f, -2.0f),
    };

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);  // sampler uzyje unit 0

    while (!glfwWindowShouldClose(window)) {
        float t = (float)glfwGetTime();
        deltaTime = t - lastFrame;
        lastFrame = t;

        processInput(window);

        glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        grassAtlas.bind(0);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(70.0f),
            (float)WINDOW_WIDTH / WINDOW_HEIGHT, 0.1f, 1000.0f);

        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO);

        for (int i = 0; i < 5; i++) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cubePositions[i]);
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}