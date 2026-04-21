#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
	Shader(const std::string& vertPath, const std::string& fragPath);
	~Shader();

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;

	void use() const;
	GLuint id() const { return m_id; }

	void setInt(const std::string& name, int value) const;
	void setMat4(const std::string& name, const glm::mat4& m) const;
	void setVec3(const std::string& name, const glm::vec3& v) const;

private:
	GLuint m_id = 0;
};