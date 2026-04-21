#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>

class Shader;

class Skybox {
public:
	Skybox();
	~Skybox();
	void draw(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& sunDir);
private:
	GLuint m_VAO = 0, m_VBO = 0;
	Shader* m_shader = nullptr;
};