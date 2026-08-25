#ifndef CAMERA_HPP
# define CAMERA_HPP

# ifndef DEBUG
#  define DEBUG 0
# endif

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

enum class CameraDirection { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera {
	public:
		Camera(glm::vec3 position);

		glm::mat4	getViewMatrix() const;
		glm::vec3	getPosition() const { return pos; }
		float		getYaw() const { return yaw; }
		float		getPitch() const { return pitch; }

		void processKeyboard(CameraDirection dir, float velocity);
		void processMouseMovement(float xoffset, float yoffset);

	private:
		void updateVectors();

		glm::vec3 pos;
		glm::vec3 front;
		glm::vec3 up;
		glm::vec3 right;
		glm::vec3 worldUp;
		float     yaw;
		float     pitch;
		float     sensitivity = 0.1f;
};

#endif