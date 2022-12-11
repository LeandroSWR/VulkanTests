#pragma once

#include "vt_window.hpp"
#include "vt_pipeline.hpp"
#include "vt_device.hpp"
#include "vt_swap_chain.hpp"

// std
#include <memory>
#include <vector>

namespace vt
{
	class FirstApp
	{
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp &operator=(const FirstApp&) = delete;

		void run();

	private:
		void createPipelineLayout();
		void createPipeline();
		void createCommandBuffers();
		void drawFrame();

		VtWindow vtWindow{ WIDTH , HEIGHT, "Hello Vulkan!" };
		VtDevice vtDevice{ vtWindow };
		VtSwapChain vtSwapChain{ vtDevice , vtWindow.getExtent() };
		std::unique_ptr<VtPipeline> vtPipeline;
		VkPipelineLayout pipelineLayout;
		std::vector<VkCommandBuffer> commandBuffers;
	};
}