#pragma once

#include <string>
#include <glm/glm.hpp>

class Shader {
public:
	Shader(const std::string& vertPath, const std::string& fragPath);
	~Shader();

	void use() const;
	void setMat4(const std::string& name, const glm::mat4& mat) const;
	void setInt(const std::string& name, int val) const;
	void setVec3(const std::string& name, const glm::vec3& v) const;
	unsigned int id() const { return id_; }

private:
	unsigned int id_;
	static unsigned int compileShader(const std::string& path, unsigned int type);
	static std::string  readFile(const std::string& path);
};