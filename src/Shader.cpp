#include "Shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <sstream>
#include <iostream>

static std::string readFile(const std::string& path) {
	std::ifstream f(path);
	if (!f.is_open()) { std::cerr << "Cannot read " << path << "\n"; return ""; }
	std::stringstream ss; ss << f.rdbuf(); return ss.str();
}

static GLuint compile(GLenum type, const std::string& src) {
	GLuint s = glCreateShader(type);
	const char* c = src.c_str();
	glShaderSource(s, 1, &c, nullptr);
	glCompileShader(s);
	GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
	if (!ok) { char log[1024]; glGetShaderInfoLog(s, 1024, nullptr, log); std::cerr << "Shader err:\n" << log << "\n"; }
	return s;
}

Shader::Shader(const std::string& vp, const std::string& fp) {
	GLuint v = compile(GL_VERTEX_SHADER, readFile(vp));
	GLuint f = compile(GL_FRAGMENT_SHADER, readFile(fp));
	m_id = glCreateProgram();
	glAttachShader(m_id, v);
	glAttachShader(m_id, f);
	glLinkProgram(m_id);
	GLint ok;
	glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
	if (!ok) { char log[1024]; glGetProgramInfoLog(m_id, 1024, nullptr, log); std::cerr << "Link err:\n" << log << "\n"; }
	glDeleteShader(v);
	glDeleteShader(f);
}

Shader::~Shader() { if (m_id) glDeleteProgram(m_id); }

void Shader::use() const { glUseProgram(m_id); }

void Shader::setInt(const std::string& n, int v) const {
	glUniform1i(glGetUniformLocation(m_id, n.c_str()), v);
}

void Shader::setMat4(const std::string& n, const glm::mat4& m) const {
	glUniformMatrix4fv(glGetUniformLocation(m_id, n.c_str()), 1, GL_FALSE, glm::value_ptr(m));
}

void Shader::setVec3(const std::string& n, const glm::vec3& v) const {
	glUniform3fv(glGetUniformLocation(m_id, n.c_str()), 1, glm::value_ptr(v));
}

void Shader::setFloat(const std::string& n, float v) const {
	glUniform1f(glGetUniformLocation(m_id, n.c_str()), v);
}