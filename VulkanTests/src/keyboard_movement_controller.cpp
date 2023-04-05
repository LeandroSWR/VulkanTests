#include "keyboard_movement_controller.hpp"

namespace vt
{
	void KeyboardMovementController::moveInPlaneXZ(GLFWwindow* window, float dt, VtGameObject& gameObject)
	{
		glm::vec3 rotate{ 0 };
		if (glfwGetKey(window, keys.lookRight) == GLFW_PRESS) rotate.y += 1.f;
		if (glfwGetKey(window, keys.lookLeft) == GLFW_PRESS) rotate.y -= 1.f;
		if (glfwGetKey(window, keys.lookUp) == GLFW_PRESS) rotate.x += 1.f;
		if (glfwGetKey(window, keys.lookDown) == GLFW_PRESS) rotate.x -= 1.f;

		if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
		{
			gameObject.transform.rotation += lookSpeed * dt * glm::normalize(rotate);
			// limit pitch values between about +/- 85ish degrees
			gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -1.5f, 1.5f);
			gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
		}

		glm::mat4 world_matrix = gameObject.transform.mat4();
		glm::vec3 forwardDir = world_matrix * glm::vec4(0, 0, 1, 0); forwardDir.y = 0; forwardDir = glm::normalize(forwardDir);
		glm::vec3 rightDir = world_matrix * glm::vec4(1, 0, 0, 0); rightDir.y = 0; rightDir = glm::normalize(rightDir);
		glm::vec3 upDir{ 0.f, 1.f, 0.f };

		glm::vec3 moveDir{ 0.f };
		if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
		if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
		if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
		if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
		if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
		if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;

		if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
		{
			gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
		}
	}
}