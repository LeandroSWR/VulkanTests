/*
04/07/2023
Abstract class that manages render passes
*/

#pragma once
#include "vt_device.hpp"
#include "vt_swap_chain.hpp"
#include "vt_pipeline.hpp"

namespace vt {
	struct VtRenderPassAttachment
	{
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VkFormat format = VK_FORMAT_UNDEFINED;

		void cleanAttachment(VkDevice device) {
			vkDestroyImage(device, image, nullptr);
			vkFreeMemory(device, deviceMemory, nullptr);
			vkDestroyImageView(device, imageView, nullptr);
		}
	};

	class VtRenderPass
	{
	public:
		VtRenderPass(VtDevice& device, std::shared_ptr<VtSwapChain> swapchain);
		virtual ~VtRenderPass();

		VtRenderPass(const VtRenderPass&) = delete;
		VtRenderPass& operator=(const VtRenderPass&) = delete;

		virtual void createRenderPass() = 0;
		virtual void createFramebuffer() = 0;
		virtual void createAttachments() = 0;
		virtual void cleanAttachments() = 0;
		virtual void recreateSwapchain(std::shared_ptr<VtSwapChain> swapchain) = 0;
		virtual void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts) = 0;
		VkPipelineLayout getPipelineLayout() {
			return pipelineLayout;
		};
		virtual void createDefaultPipeline() = 0;
		void bindDefaultPipeline(VkCommandBuffer commandBuffer) {
			vtPipeline->bind(commandBuffer);
		}
		virtual void updatePipelineRessources() = 0;
		virtual void createPipelineRessources() = 0;
		virtual void startRenderPass(VkCommandBuffer commandBuffer, int currentImageIndex) = 0;
		virtual void endRenderPass(VkCommandBuffer commandBuffer, int imageIndex) = 0;
		[[nodiscard]] VkRenderPass getRenderPass();
		[[nodiscard]] VkFramebuffer getFramebuffer(int index);
		void cleanFramebuffer();
	protected:
		VtDevice& device;
		std::shared_ptr<VtSwapChain> swapchain;
		VkRenderPass renderPass = VK_NULL_HANDLE;
		std::vector<VkFramebuffer> framebuffers;

		VkPipelineLayout pipelineLayout;
		std::unique_ptr<VtPipeline> vtPipeline;


	};
}


