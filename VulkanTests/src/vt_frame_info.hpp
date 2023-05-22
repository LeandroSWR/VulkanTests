#pragma once

#include "vt_camera.hpp"
#include "vt_game_object.hpp"

namespace vt 
{
	#define MAX_LIGHTS 10

	struct DirectionalLight // For later use
	{
		glm::mat4 mvp{ 1.0 };
		glm::vec4 position{ 0.0f, 0.0f, 0.0f, 0.0f };
	};

	struct PointLight
	{
		glm::vec4 position{};  // ignore w
		glm::vec4 color{};     // w is intensity
	};

	struct GlobalUbo
	{
		glm::mat4 projection{ 1.f };
		glm::mat4 view{ 1.f };
		glm::mat4 iverseView{ 1.f };
		glm::vec4 ambientLightColor{ 1.f, 1.f, 1.f, .02f };  // w is intensity
		PointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	struct FrameInfo
	{
		int frame_index;
		float frame_time;
		VkCommandBuffer command_buffer;
		VtCamera& camera;
		VkDescriptorSet global_descriptor_set;
		VtGameObject::Map& game_objects;
		GlobalUbo ubo;
	};
}