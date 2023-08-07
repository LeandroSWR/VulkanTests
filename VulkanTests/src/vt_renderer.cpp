#include "vt_renderer.hpp"

// std
#include <stdexcept>
#include <array>

namespace vt
{
	VtRenderer::VtRenderer(VtWindow& window, VtDevice& device) : vtWindow{ window }, vtDevice{device}
	{
		recreateSwapChain();
		createCommandBuffers();
	}

	VtRenderer::~VtRenderer()
	{
		freeCommandBuffers();
	}

	void VtRenderer::recreateSwapChain()
	{
		auto extent = vtWindow.getExtent();
		while (extent.width == 0 || extent.height == 0) {
			extent = vtWindow.getExtent();
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(vtDevice.device());

		if (vtSwapChain == nullptr) {
			vtSwapChain = std::make_shared<VtSwapChain>(vtDevice, extent);
		}
		else
		{
			std::shared_ptr<VtSwapChain> oldSwapChain = std::move(vtSwapChain);
			vtSwapChain = std::make_shared<VtSwapChain>(vtDevice, extent, oldSwapChain);
			
			if (!oldSwapChain->compareSwapFormats(*vtSwapChain.get()))
			{
				throw std::runtime_error("Swap chain image(or depth) format has changed!");
			}
		}
		

		// Come back here
	}

	void VtRenderer::createCommandBuffers()
	{
		commandBuffers.resize(VtSwapChain::MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = vtDevice.getCommandPool();
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(vtDevice.device(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}

	void VtRenderer::freeCommandBuffers()
	{
		vkFreeCommandBuffers(
			vtDevice.device(),
			vtDevice.getCommandPool(),
			static_cast<uint32_t>(commandBuffers.size()),
			commandBuffers.data());

		commandBuffers.clear();
	}

	VkCommandBuffer VtRenderer::beginFrame()
	{
		assert(!isFrameStarted && "Can't call beginFrame while already in progress");

		auto result = vtSwapChain->acquireNextImage(&currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			recreateSwapChain();
			return nullptr;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		isFrameStarted = true;

		auto commandBuffer = getCurrentCommandBuffer();
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to to begin recording command buffer!");
		}

		return commandBuffer;
	}

	//False : recreate window size
	bool VtRenderer::endFrame()
	{
		bool ret = true;
		assert(isFrameStarted && "Can't call endFrame while frame is not in progress");
		
		auto commandBuffer = getCurrentCommandBuffer();
		
		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to record command buffer!");
		}

		auto result = vtSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || vtWindow.wasWindowResized()) {
			vtWindow.resetWindowResizedFlag();
			recreateSwapChain();
			ret = false;
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % VtSwapChain::MAX_FRAMES_IN_FLIGHT;
		return ret;
	}



}