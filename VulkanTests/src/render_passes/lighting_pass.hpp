#pragma once
#include "../vt_device.hpp"
#include "gbuffer_pass.hpp"
#include "../vt_descriptors.hpp"
#include "glm\glm.hpp"

namespace vt {
	class LightingPass : public VtRenderPass
	{
	public:
		LightingPass(VtDevice& deviceRef, std::shared_ptr<VtSwapChain> swapchainRef, std::vector<VkDescriptorSetLayout> descriptorSetLayouts, std::shared_ptr<GBufferPass> gBufferPass);
		virtual ~LightingPass()override;

		LightingPass(const LightingPass&) = delete;
		LightingPass& operator=(const LightingPass&) = delete;

		virtual void createRenderPass()override;
		virtual void createFramebuffer()override;
		virtual void createAttachments()override;
		virtual void cleanAttachments()override;
		virtual void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts)override;
		virtual void createDefaultPipeline()override;
		virtual void createPipelineRessources()override;
		virtual void startRenderPass(VkCommandBuffer commandBuffer, int currentImage)override;
		virtual void endRenderPass(VkCommandBuffer commandBuffer, int imageIndex)override;
		virtual void updatePipelineRessources()override {};
		VkImageView getLightingAttachment(int imageIndex);
		virtual void recreateSwapchain(std::shared_ptr<VtSwapChain> swapchain);

	private:
		const std::string LIGHTING_PASS_VERTEX_SHADER_PATH = "shaders/light_shader.vert.spv";
		const std::string LIGHTING_PASS_FRAGMENT_SHADER_PATH = "shaders/light_shader.frag.spv";

		std::shared_ptr<GBufferPass> gBufferPass;
		std::vector<VtRenderPassAttachment> outLightingAttachment;

		std::unique_ptr<VtDescriptorPool> gBufferTexturesDescriptorPool;
		std::unique_ptr<VtDescriptorSetLayout> gBufferTexturesDescriptorSetLayout;
		std::vector<VkDescriptorSet> gBufferTexturesDescriptorSets;
		VkSampler gBufferSampler;

		void createGBufferTexturesDescriptorSet();
	};

}


