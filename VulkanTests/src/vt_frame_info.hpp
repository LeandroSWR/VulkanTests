#pragma once

#include "vt_camera.hpp"
#include "vt_game_object.hpp"

namespace vt 
{
#define MAX_LIGHTS 10

	struct PointLight
	{
		glm::vec4 position{};  // ignore w
		glm::vec4 color{};     // w is intensity
	};

	struct GlobalUbo
	{
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 inverseView{ 1.f };
		glm::mat4 inverseProjection{ 1.f };
		glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };  // w is intensity
		PointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	struct FrameInfo
	{
		int frameIndex;
		int imageIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		VtCamera& camera;
		VkDescriptorSet globalDescriptorSet;
		VtGameObject::Map& gameObjects;
	};
}