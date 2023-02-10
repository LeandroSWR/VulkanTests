#pragma once

#include "vt_device.hpp"
#include "vt_model.hpp"
#include "vt_swap_chain.hpp"
#include "vt_window.hpp"

// std
#include <memory>
#include <vector>
#include <cassert>

namespace vt
{
	class VtRenderer
	{
	public:
		VtRenderer(VtWindow& window, VtDevice& device);
		~VtRenderer();

		VtRenderer(const VtRenderer&) = delete;
		VtRenderer& operator=(const VtRenderer&) = delete;

		VkRenderPass getSwapChainRenderPass() const { return vtSwapChain->getRenderPass(); }
		float getAspectRatio() const { return vtSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
			return commandBuffers[currentFrameIndex];
		}

		int getFrameIndex() const
		{
			assert(isFrameStarted && "Cannot get frame index when frame not in progress");
			return currentFrameIndex;
		}

		VkCommandBuffer beginFrame();
		void endFrame();
		void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
		void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		VtWindow& vtWindow;
		VtDevice& vtDevice;
		std::unique_ptr<VtSwapChain> vtSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex{0};
		bool isFrameStarted{false};
	};
}