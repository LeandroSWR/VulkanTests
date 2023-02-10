#pragma once

#include "vt_camera.hpp"
#include "vt_device.hpp"
#include "vt_model.hpp"
#include "vt_game_object.hpp"
#include "vt_pipeline.hpp"

// std
#include <memory>
#include <vector>

namespace vt
{
	class SimpleRenderSystem
	{
	public:
		SimpleRenderSystem(VtDevice &device, VkRenderPass renderPass);
		~SimpleRenderSystem();

		SimpleRenderSystem(const SimpleRenderSystem&) = delete;
		SimpleRenderSystem& operator=(const SimpleRenderSystem&) = delete;

		void renderGameObjects(VkCommandBuffer commandBuffer, std::vector<VtGameObject>& gameObjects, const VtCamera& camera);

	private:
		void createPipelineLayout();
		void createPipeline(VkRenderPass renderPass);
		
		VtDevice& vtDevice;

		std::unique_ptr<VtPipeline> vtPipeline;
		VkPipelineLayout pipelineLayout;
	};
}