#include "Shader.hpp"
#include <GL/glew.h>
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::string Shader::readFile(const std::string& path) {
	std::ifstream file(path);
	if (!file.is_open())
		throw std::runtime_error("Cannot open shader: " + path);
	std::ostringstream ss;
	ss << file.rdbuf();
	return ss.str();
}

unsigned int Shader::compileShader(const std::string& path, unsigned int type) {
	std::string  src  = readFile(path);
	const char*  csrc = src.c_str();
	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &csrc, nullptr);
	glCompileShader(shader);
	int ok;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetShaderInfoLog(shader, 512, nullptr, log);
		throw std::runtime_error(std::string("Shader compile error: ") + log);
	}
	return shader;
}

Shader::Shader(const std::string& vertPath, const std::string& fragPath) {
	unsigned int vert = compileShader(vertPath, GL_VERTEX_SHADER);
	unsigned int frag = compileShader(fragPath, GL_FRAGMENT_SHADER);
	id_ = glCreateProgram();
	glAttachShader(id_, vert);
	glAttachShader(id_, frag);
	glLinkProgram(id_);
	glDeleteShader(vert);
	glDeleteShader(frag);
	int ok;
	glGetProgramiv(id_, GL_LINK_STATUS, &ok);
	if (!ok) {
		char log[512];
		glGetProgramInfoLog(id_, 512, nullptr, log);
		throw std::runtime_error(std::string("Shader link error: ") + log);
	}
}

Shader::~Shader() {
	glDeleteProgram(id_);
}

void Shader::use() const { glUseProgram(id_); }

void Shader::setMat4(const std::string& name, const glm::mat4& mat) const {
	glUniformMatrix4fv(glGetUniformLocation(id_, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setInt(const std::string& name, int val) const {
	glUniform1i(glGetUniformLocation(id_, name.c_str()), val);
}

void Shader::setVec3(const std::string& name, const glm::vec3& v) const {
	glUniform3fv(glGetUniformLocation(id_, name.c_str()), 1, glm::value_ptr(v));
}