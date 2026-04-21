#pragma once
#include <string>
#include <glad/glad.h>

class Texture {
public:
	explicit Texture(const std::string& path);

	Texture();
	~Texture();
	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	void bind(GLuint uint = 0) const;
	GLuint id() const { return m_id; }

private:
	GLuint m_id = 0;
};