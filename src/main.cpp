#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

// Helper function read whole file to string
std::string loadFile(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open()) {
		std::cerr << "Cannot read the file: " << path << "\n";
		return "";
	}
	std::stringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

// Compilation of one shader

GLuint compileShader(GLenum type, const std::string& source) {
	GLuint shader = glCreateShader(type);
	const char* src = source.c_str();
	glShaderSource(shader, 1, &src, nullptr);
	glCompileShader(shader);

	//Checking compilation errors
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[512];
		glGetShaderInfoLog(shader, 512, nullptr, log);
		std::cerr << "Compilation shader error:\n" << log << "\n";

	}
	return shader;
}

// Creating shader program

GLuint createShaderProgram(const std::string& vertPath, const std::string& fragPath) {
	std::string vertSrc = loadFile(vertPath);
	std::string fragSrc = loadFile(fragPath);

	GLuint vert = compileShader(GL_VERTEX_SHADER, vertSrc);
	GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragSrc);

	GLuint program = glCreateProgram();
	glAttachShader(program, vert);
	glAttachShader(program, frag);
	glLinkProgram(program);

	// Checking linking errors
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		char log[512];
		glGetProgramInfoLog(program, 512, nullptr, log);
		std::cerr << "Error linking shader program:\n" << log << "\n";
	}

	glDeleteShader(vert);
	glDeleteShader(frag);

	return program;
}

// Callbacks

void framebuffer_size_callback(GLFWwindow*, int width, int height) {
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

// Main

int main() {
	// GLFW init

	if (!glfwInit()) {
		std::cerr << "Couldn't initialize GLFW\n";
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "CubeCraft - First triangle", nullptr, nullptr);
	if (!window) {
		std::cerr << "Couldn't open window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Couldn't load GLAD\n";
		return -1;
	}
	
	std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
	std::cout << "GPU: " << glGetString(GL_RENDERER) << "\n";

	float vertices[] = {
		-0.5f, -0.5f, 0.0f,		0.0f, 1.0f, 0.0f,
		 0.5f, -0.5f, 0.0f,		0.0f, 0.0f, 1.0f,
		 0.0f,  0.5f, 0.0f,		1.0f, 0.0f, 0.0f
	};

	// VAO: keeps the config how to read data
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// VBO: buffor with vertices data on GPU
	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Atribute 0: position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Atribute 1: color
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
		(void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Shader
	GLuint shaderProgram = createShaderProgram("shaders/basic.vert","shaders/basic.frag");

	// Main loop
	while (!glfwWindowShouldClose(window)) {
		processInput(window);

		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//Traingle drawing
		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// Cleaning
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProgram);

	glfwTerminate();
	return 0;
}