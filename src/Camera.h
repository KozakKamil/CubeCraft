#pragma once
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class Camera {
public:
	glm::vec3 position{ 0.0f,20.0f,30.0f };
	glm::vec3 front{ 0.0f, 0.0f,-1.0f };
	glm::vec3 up{ 0.0f,1.0f,0.0f };

	float yaw = -90.0f;
	float pitch = 0.0f;
	float speed = 10.0f;
	float sensitivity = 0.1f;

	glm::mat4 viewMatrix() const;
	void processKeyboard(GLFWwindow* window, float deltaTime);
	void processMouse(float xoffset, float yoffset);
};