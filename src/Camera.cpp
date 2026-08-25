#include "../include/Camera.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position)
	: pos(position), worldUp(0.0f, 1.0f, 0.0f), yaw(-90.0f), pitch(0.0f)
{
	updateVectors();
}

glm::mat4 Camera::getViewMatrix() const {
	return glm::lookAt(pos, pos + front, up);
}

void Camera::processKeyboard(CameraDirection dir, float velocity) {
	if (dir == CameraDirection::FORWARD)  pos += front * velocity;
	if (dir == CameraDirection::BACKWARD) pos -= front * velocity;
	if (dir == CameraDirection::LEFT)     pos -= right * velocity;
	if (dir == CameraDirection::RIGHT)    pos += right * velocity;
}

void Camera::processMouseMovement(float xoffset, float yoffset) {
	yaw   += xoffset * sensitivity;
	pitch += yoffset * sensitivity;
	if (pitch >  89.0f) pitch =  89.0f;
	if (pitch < -89.0f) pitch = -89.0f;
	updateVectors();
}

void Camera::updateVectors() {
	glm::vec3 front_vec;
	front_vec.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
	front_vec.y = std::sin(glm::radians(pitch));
	front_vec.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
	front  = glm::normalize(front_vec);
	right  = glm::normalize(glm::cross(front, worldUp));
	up     = glm::normalize(glm::cross(right, front));
}