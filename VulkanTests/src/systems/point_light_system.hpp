#pragma once

#include "vt_camera.hpp"
#include "vt_device.hpp"
#include "vt_model.hpp"
#include "vt_frame_info.hpp"
#include "vt_game_object.hpp"
#include "vt_pipeline.hpp"

// std
#include <memory>
#include <vector>

namespace vt
{
	class PointLightSystem
	{
	public:
		PointLightSystem(VtDevice &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
		~PointLightSystem();

		PointLightSystem(const PointLightSystem&) = delete;
		PointLightSystem& operator=(const PointLightSystem&) = delete;

		void update(FrameInfo& frameInfo);
		void render(FrameInfo& frameInfo);

	private:
		void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
		void createPipeline(VkRenderPass renderPass);
		
		VtDevice& vtDevice;

		std::unique_ptr<VtPipeline> vtPipeline;
		VkPipelineLayout pipelineLayout;
	};
}