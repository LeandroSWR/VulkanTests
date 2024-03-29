#pragma once

#include "../vt_camera.hpp"
#include "../vt_device.hpp"
#include "../vt_model.hpp"
#include "../vt_frame_info.hpp"
#include "../vt_game_object.hpp"
#include "../vt_pipeline.hpp"

// std
#include <memory>
#include <vector>

namespace vt
{
	class SimpleRenderSystem
	{
	public:
		SimpleRenderSystem(VtDevice &device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> setLayouts);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(FrameInfo& frameInfo);

	private:
		
		VtDevice& vtDevice;

		std::unique_ptr<VtPipeline> vtPipeline;
		VkPipelineLayout pipelineLayout;
	};
}