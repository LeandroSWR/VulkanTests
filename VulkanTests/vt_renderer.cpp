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
			vtSwapChain = std::make_unique<VtSwapChain>(vtDevice, extent);
		}
		else
		{
			std::shared_ptr<VtSwapChain> oldSwapChain = std::move(vtSwapChain);
			vtSwapChain = std::make_unique<VtSwapChain>(vtDevice, extent, oldSwapChain);
			
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

	void VtRenderer::endFrame()
	{
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
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}

		isFrameStarted = false;
		currentFrameIndex = (currentFrameIndex + 1) % VtSwapChain::MAX_FRAMES_IN_FLIGHT;
	}

	void VtRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(isFrameStarted && "Can't call beginSwapChainRenderPass if frame is not in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't begin render pass on command buffer from a different frame");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = vtSwapChain->getRenderPass();
		renderPassInfo.framebuffer = vtSwapChain->getFrameBuffer(currentImageIndex);

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = vtSwapChain->getSwapChainExtent();

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { 0.01f, 0.01f, 0.01f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(vtSwapChain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(vtSwapChain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, vtSwapChain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void VtRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer)
	{
		assert(isFrameStarted && "Can't call endSwapChainRenderPass if frame is not in progress");
		assert(commandBuffer == getCurrentCommandBuffer() && "Can't end render pass on command buffer from a different frame");

		vkCmdEndRenderPass(commandBuffer);
	}
}