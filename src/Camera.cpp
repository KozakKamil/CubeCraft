#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::viewMatrix() const {
	return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(GLFWwindow* w, float dt) {
	float s = speed * dt;
	if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) s *= 3.0f;
	if (glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) position += s * front;
	if (glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) position -= s * front;
	if (glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) position -= glm::normalize(glm::cross(front, up)) * s;
	if (glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) position += glm::normalize(glm::cross(front, up)) * s;
	if (glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS) position += s * up;
	if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) position -= s * up;
}

void Camera::processMouse(float dx, float dy) {
	yaw += dx * sensitivity;
	pitch += dy * sensitivity;
	if (pitch > 89.0f) pitch = 89.0f;
	if (pitch < -89.0f) pitch = -89.0f;

	glm::vec3 f;
	f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	f.y = sin(glm::radians(pitch));
	f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	front = glm::normalize(f);
}