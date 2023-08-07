#pragma once
#include "../vt_device.hpp"
#include "../vt_render_pass.hpp"
#include "../vt_descriptors.hpp"
#include "glm\glm.hpp"

namespace vt {
	struct SimplePushConstantData
	{
		glm::mat4 modelMatrix{ 1.0f };
		glm::mat4 normalMatrix{ 1.0f };
	};

	class GBufferPass :
		public VtRenderPass
	{
	public:
		GBufferPass(VtDevice& deviceRef, std::shared_ptr<VtSwapChain> swapchainRef, std::vector<VkDescriptorSetLayout> descriptorSetLayouts);
		virtual ~GBufferPass()override;

		GBufferPass(const GBufferPass&) = delete;
		GBufferPass& operator=(const GBufferPass&) = delete;

		virtual void createRenderPass()override;
		virtual void createFramebuffer()override;
		virtual void createAttachments()override;
		virtual void cleanAttachments()override;
		virtual void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts)override;
		virtual void createDefaultPipeline()override;
		virtual void updatePipelineRessources()override;
		virtual void createPipelineRessources()override;
		virtual void startRenderPass(VkCommandBuffer commandBuffer, int currentImageIndex)override;
		virtual void endRenderPass(VkCommandBuffer commandBuffer, int imageIndex)override;
		VkImageView getAlbedoAttachment(uint32_t imageIndex);
		VkImageView getPositionAttachment(uint32_t imageIndex);
		VkImageView getNormalAttachment(uint32_t imageIndex);
		VkImageView getDepthAttachment(uint32_t imageIndex);
		virtual void recreateSwapchain(std::shared_ptr<VtSwapChain> swapchain);
	private:
		const std::string G_BUFFER_PASS_VERTEX_SHADER_PATH = "shaders/g_buffer_shader.vert.spv";
		const std::string G_BUFFER_PASS_FRAGMENT_SHADER_PATH = "shaders/g_buffer_shader.frag.spv";

		std::vector<VtRenderPassAttachment> albedoRoughnessAttachments;
		std::vector<VtRenderPassAttachment> normalMetallicAttachments;
		std::vector<VtRenderPassAttachment> positionAttachments;
		std::vector<VtRenderPassAttachment> depthAttachments;
	};
}

