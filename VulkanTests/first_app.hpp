#pragma once

#include "vt_pipeline.hpp"
#include "vt_swap_chain.hpp"
#include "vt_window.hpp"
#include "vt_model.hpp"
#include "vt_device.hpp"

// std
#include <memory>
#include <vector>

namespace vt
{
	class FirstApp
	{
	public:
		static constexpr int WIDTH = 1024;
		static constexpr int HEIGHT = 768;

		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp &operator=(const FirstApp&) = delete;

		void run();

	private:
		void loadModels();
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffers();
		void freeCommandBuffers();
		void drawFrame();
		void recreateSwapChain();
		void recordCommandBuffer(int imageIndex);

		VtWindow vtWindow{ WIDTH , HEIGHT, "Hello Vulkan!" };
		VtDevice vtDevice{ vtWindow };
		std::unique_ptr<VtSwapChain> vtSwapChain;
		std::unique_ptr<VtPipeline> vtPipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
		std::unique_ptr<VtModel> vtModel;
	};
}