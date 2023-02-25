#pragma once

#include "vt_camera.hpp"

// lib
#include <vulkan/vulkan.h>

namespace vt 
{
	struct FrameInfo
	{
		int frameIndex;
		float frameTime;
		VkCommandBuffer commandBuffer;
		VtCamera& camera;
		VkDescriptorSet globalDescriptorSet;
	};
}