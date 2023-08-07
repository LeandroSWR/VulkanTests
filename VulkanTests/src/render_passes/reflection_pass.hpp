#pragma once
#include "../vt_device.hpp"
#include "gbuffer_pass.hpp"
#include "lighting_pass.hpp"
#include "../vt_descriptors.hpp"
#include "glm\glm.hpp"

namespace vt {
	class ReflectionPass : public VtRenderPass
	{
	public:
		ReflectionPass(VtDevice& deviceRef, std::shared_ptr<VtSwapChain> swapchainRef, std::vector<VkDescriptorSetLayout> descriptorSetLayouts, std::shared_ptr<GBufferPass> gBufferPass, std::shared_ptr<LightingPass> lightingPass);
		virtual ~ReflectionPass()override;

		ReflectionPass(const ReflectionPass&) = delete;
		ReflectionPass& operator=(const ReflectionPass&) = delete;

		virtual void createRenderPass()override;
		virtual void createFramebuffer()override;
		virtual void createAttachments()override;
		virtual void cleanAttachments()override;
		virtual void createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts)override;
		virtual void createDefaultPipeline()override;
		virtual void createPipelineRessources()override;
		virtual void startRenderPass(VkCommandBuffer commandBuffer, int currentImageIndex)override;
		virtual void endRenderPass(VkCommandBuffer commandBuffer, int imageIndex)override;
		virtual void updatePipelineRessources()override {};

		virtual void recreateSwapchain(std::shared_ptr<VtSwapChain> swapchain);

	private:
		const std::string LIGHTING_PASS_VERTEX_SHADER_PATH = "shaders/ssr_shader.vert.spv";
		const std::string LIGHTING_PASS_FRAGMENT_SHADER_PATH = "shaders/ssr_shader.frag.spv";

		std::shared_ptr<GBufferPass> gBufferPass;
		std::shared_ptr<LightingPass> lightingPass;
		std::vector<VtRenderPassAttachment> outReflectionAttachment;
		std::vector<VtRenderPassAttachment> outReflectionDebugAttachment;

		std::unique_ptr<VtDescriptorPool> gBufferTexturesDescriptorPool;
		std::unique_ptr<VtDescriptorSetLayout> gBufferTexturesDescriptorSetLayout;
		std::vector<VkDescriptorSet> gBufferTexturesDescriptorSets;
		VkSampler gBufferSampler;

		void createGBufferTexturesDescriptorSet();
	};

}


