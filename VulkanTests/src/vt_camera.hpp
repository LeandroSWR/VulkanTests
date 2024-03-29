#pragma once

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace vt
{
	class VtCamera
	{
	public:
		void setOrthographicProjection(
			float left, float right, float top, float bottom, float near, float far);
		void setPerspectiveProjection(float fovy, float width, float height, float near, float far);

		void setViewDirection(
			glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });
		void setViewTarget(
			glm::vec3 position, glm::vec3 direction, glm::vec3 up = glm::vec3{ 0.f, -1.f, 0.f });
		void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

		const glm::mat4& getInverseView() const { return inverseViewMatrix; }
		const glm::mat4& getInverseProjection() const { return inverseProjectionMatrix; }
		const glm::mat4& getProjection() const { return projectionMatrix; }
		const glm::mat4& getView() const { return viewMatrix; }

		void setView(const glm::mat4& world_matrix);

	private:
		glm::mat4 projectionMatrix{ 1.f };
		glm::mat4 viewMatrix{ 1.f };
		glm::mat4 inverseViewMatrix{ 1.f };
		glm::mat4 inverseProjectionMatrix{ 1.f };
	};
}