#pragma once

#include "vt_device.hpp"
#include "vt_model.hpp"
#include "vt_swap_chain.hpp"
#include "vt_window.hpp"
#include "vt_render_pass.hpp"

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

		float getAspectRatio() const { return vtSwapChain->extentAspectRatio(); }
		bool isFrameInProgress() const { return isFrameStarted; }
		std::shared_ptr<VtSwapChain> getSwapchain() const {
			return vtSwapChain;
		};

		VkCommandBuffer getCurrentCommandBuffer() const {
			assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
			return commandBuffers[currentFrameIndex];
		}

		int getImageIndex() const 
		{
			return currentImageIndex;
		}

		int getFrameIndex() const
		{
			assert(isFrameStarted && "Cannot get frame index when frame not in progress");
			return currentFrameIndex;
		}

		VkCommandBuffer beginFrame();
		bool endFrame();

	private:
		void createCommandBuffers();
		void freeCommandBuffers();
		void recreateSwapChain();

		VtWindow& vtWindow;
		VtDevice& vtDevice;
		std::shared_ptr<VtSwapChain> vtSwapChain;
		std::vector<VkCommandBuffer> commandBuffers;

		uint32_t currentImageIndex;
		int currentFrameIndex{0};
		bool isFrameStarted{false};
	};
}