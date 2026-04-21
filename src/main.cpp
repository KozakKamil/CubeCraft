#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw = -90.0f;
float pitch = 0.0f;

float lastMouseX = WINDOW_WIDTH / 2.0f;
float lastMouseY = WINDOW_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;


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
	GLint success = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success != GL_TRUE) {
		GLint logLen = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);

		std::string log(logLen, '\0');
		glGetShaderInfoLog(shader, logLen, nullptr, log.data());

		std::cerr << "Compilation shader error:\n" << log << "\n";

		glDeleteShader(shader);
		return 0;
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

void mouse_callback(GLFWwindow*, double xpos, double ypos) {
	if (firstMouse) {
		lastMouseX = (float)xpos;
		lastMouseY = (float)ypos;
		firstMouse = false;
	}

	float xoffset = (float)xpos - lastMouseX;
	float yoffset = lastMouseY - (float)ypos;
	lastMouseX = (float)xpos;
	lastMouseY = (float)ypos;

	float sensitivity = 0.1f;
	xoffset *= sensitivity;
	yoffset *= sensitivity;

	yaw += xoffset;
	pitch += yoffset;

	if (pitch > 89.0f) {
		pitch = 89.0f;
	}

	if (pitch < -89.0f) {
		pitch = -89.0f;
	}

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	cameraFront = glm::normalize(front);
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	float speed = 5.0f * deltaTime;
	if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		cameraPos += speed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		cameraPos -= speed * cameraFront;
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		cameraPos += speed * cameraUp;
	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		cameraPos -= speed * cameraUp;
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

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "CubeCraft - 3D Cube", nullptr, nullptr);
	if (!window) {
		std::cerr << "Couldn't open window\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Couldn't load GLAD\n";
		return -1;
	}
	
	std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
	std::cout << "GPU: " << glGetString(GL_RENDERER) << "\n";

	float vertices[] = {
		// pozycja              // kolor
		// Tyl (Z = -0.5) - czerwony
		-0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
		-0.5f,  0.5f, -0.5f,    1.0f, 0.0f, 0.0f,
		-0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 0.0f,

		// Przod (Z = 0.5) - zielony
		-0.5f, -0.5f,  0.5f,    0.0f, 1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,    0.0f, 1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,
		-0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 0.0f,
		-0.5f, -0.5f,  0.5f,    0.0f, 1.0f, 0.0f,

		// Lewo (X = -0.5) - niebieski
		-0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
		-0.5f,  0.5f, -0.5f,    0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,    0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f, -0.5f,    0.0f, 0.0f, 1.0f,
		-0.5f, -0.5f,  0.5f,    0.0f, 0.0f, 1.0f,
		-0.5f,  0.5f,  0.5f,    0.0f, 0.0f, 1.0f,

		// Prawo (X = 0.5) - zolty
		 0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, -0.5f,    1.0f, 1.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,    1.0f, 1.0f, 0.0f,
		 0.5f, -0.5f, -0.5f,    1.0f, 1.0f, 0.0f,
		 0.5f, -0.5f,  0.5f,    1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f,  0.5f,    1.0f, 1.0f, 0.0f,

		 // Dol (Y = -0.5) - magenta
		 -0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 1.0f,
		  0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 1.0f,
		  0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 1.0f,
		  0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 1.0f,
		 -0.5f, -0.5f,  0.5f,    1.0f, 0.0f, 1.0f,
		 -0.5f, -0.5f, -0.5f,    1.0f, 0.0f, 1.0f,

		 // Gora (Y = 0.5) - cyjan
		 -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 1.0f,
		  0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 1.0f,
		  0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 1.0f,
		  0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 1.0f,
		 -0.5f,  0.5f,  0.5f,    0.0f, 1.0f, 1.0f,
		 -0.5f,  0.5f, -0.5f,    0.0f, 1.0f, 1.0f
	};

	GLuint VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	GLuint shaderProgram = createShaderProgram("shaders/basic.vert", "shaders/basic.frag");

	glm::vec3 cubePostions[] = {
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(2.0f, 0.0f, -3.0f),
		glm::vec3(-2.5f, 1.0f, -5.0f),
		glm::vec3(1.5f, -1.0f, -7.0f),
		glm::vec3(-1.0f, 0.5f,-2.0f)
	};

	// Main loop
	while (!glfwWindowShouldClose(window)) {

		float currentFrame = (float)glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		processInput(window);

		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(shaderProgram);

		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

		glm::mat4 projection = glm::perspective(
			glm::radians(70.0f),
			(float)WINDOW_WIDTH / WINDOW_HEIGHT,
			0.1f, 1000.0f
		);

		GLint viewLoc = glGetUniformLocation(shaderProgram, "uView");
		GLint projLoc = glGetUniformLocation(shaderProgram, "uProjection");
		GLint modelLoc = glGetUniformLocation(shaderProgram, "uModel");

		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
		glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

		glBindVertexArray(VAO);

		for (int i = 0; i < 5; i++) {
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, cubePostions[i]);

			model = glm::rotate(model, (float)glfwGetTime() * glm::radians(20.0f * (i + 1)),
				glm::vec3(0.5f, 1.0f, 0.3f));

			glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}


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