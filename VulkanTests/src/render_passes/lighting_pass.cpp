#include "lighting_pass.hpp"
#include <array>

namespace vt
{
	LightingPass::LightingPass(VtDevice& deviceRef, std::shared_ptr<VtSwapChain> swapchainRef, std::vector<VkDescriptorSetLayout> descriptorSetLayouts, std::shared_ptr<GBufferPass> gBufferPass) : VtRenderPass(deviceRef, swapchainRef)
	{
		this->gBufferPass = gBufferPass;
		createGBufferTexturesDescriptorSet();
		createPipelineLayout(descriptorSetLayouts);
		createAttachments();
		createRenderPass();
		createFramebuffer();
		createDefaultPipeline();
	}

	LightingPass::~LightingPass()
	{
		cleanAttachments();
		vkDestroySampler(device.device(), gBufferSampler, nullptr);
	}

	void LightingPass::createRenderPass()
	{
		VkAttachmentDescription outLightingAttachmentDescription{};
		outLightingAttachmentDescription.format = swapchain->getSwapChainImageFormat();
		outLightingAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		outLightingAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //No need to clear if we fill the screen with a quad (optimization)
		outLightingAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; //Store images meant for further use
		outLightingAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		outLightingAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		outLightingAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		outLightingAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference outLightingRef = {};
		outLightingRef.attachment = 0;
		outLightingRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &outLightingRef;


		std::array<VkSubpassDependency, 2> dependencies;
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.pAttachments = &outLightingAttachmentDescription;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass));
	}

	void LightingPass::createFramebuffer()
	{
		framebuffers.resize(swapchain->imageCount());
		for (size_t i = 0; i < swapchain->imageCount(); i++)
		{
			std::vector<VkImageView> attachments = { outLightingAttachment[i].imageView };

			VkExtent2D swapChainExtent = swapchain->getSwapChainExtent();
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(
				device.device(),
				&framebufferInfo,
				nullptr,
				&framebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	void LightingPass::createAttachments()
	{
		//Images
		VkExtent2D extent = swapchain->getSwapChainExtent();
		outLightingAttachment.resize(swapchain->imageCount());

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = swapchain->getSwapChainImageFormat();
		imageInfo.extent.width = extent.width;
		imageInfo.extent.height = extent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; //Sampled for further use as a texture



		for (size_t i = 0; i < swapchain->imageCount(); i++) {
			outLightingAttachment[0].format = imageInfo.format;
			device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outLightingAttachment[i].image, outLightingAttachment[i].deviceMemory);
		}

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.format = outLightingAttachment[0].format;

		for (size_t i = 0; i < swapchain->imageCount(); i++) {
			viewInfo.image = outLightingAttachment[i].image;
			VK_CHECK_RESULT(vkCreateImageView(device.device(), &viewInfo, nullptr, &outLightingAttachment[i].imageView));
		}

	}

	void LightingPass::cleanAttachments()
	{
		VkDevice vkDevice = device.device();
		for (size_t i = 0; i < swapchain->imageCount(); i++)
		{
			outLightingAttachment[i].cleanAttachment(vkDevice);
		}
	}

	void LightingPass::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts)
	{
		std::vector<VkDescriptorSetLayout> layouts = descriptorSetLayouts;
		layouts.push_back(gBufferTexturesDescriptorSetLayout->getDescriptorSetLayout());

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
		pipelineLayoutInfo.pSetLayouts = layouts.data();
		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void LightingPass::createDefaultPipeline()
	{
		PipelineConfigInfo pipelineConfig{};
		VtPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
		pipelineConfig.attributeDescriptions = {}; //No vertex attribute is expected in the shader (optimization)

		vtPipeline = std::make_unique<VtPipeline>(
			device,
			LIGHTING_PASS_VERTEX_SHADER_PATH,
			LIGHTING_PASS_FRAGMENT_SHADER_PATH,
			pipelineConfig);
	}


	void LightingPass::createPipelineRessources()
	{
		createGBufferTexturesDescriptorSet();
	}

	void LightingPass::startRenderPass(VkCommandBuffer commandBuffer, int currentImageIndex)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffers[currentImageIndex];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchain->getSwapChainExtent();

		renderPassInfo.clearValueCount = 0;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapchain->getSwapChainExtent().width);
		viewport.height = static_cast<float>(swapchain->getSwapChainExtent().height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		VkRect2D scissor{ {0, 0}, swapchain->getSwapChainExtent() };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		//This render pass is expected to do only one thing so we can bind the owned descriptor sets when starting the pass. Note that it is bound at index 2 only so that other descriptors (global) can be bound before without issues
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &gBufferTexturesDescriptorSets[currentImageIndex], 0, nullptr);
	}

	void LightingPass::endRenderPass(VkCommandBuffer commandBuffer, int imageIndex)
	{
		vkCmdEndRenderPass(commandBuffer);

		//Transitionning Lighting image for texture use in SSR pass (not doing it result in layout mismatch and image corruption)
		VkImageMemoryBarrier lightingMemoryBarrier{};
		lightingMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		lightingMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		lightingMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		lightingMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		lightingMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		lightingMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		lightingMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		lightingMemoryBarrier.subresourceRange.layerCount = 1;
		lightingMemoryBarrier.subresourceRange.baseMipLevel = 0;
		lightingMemoryBarrier.subresourceRange.levelCount = 1;
		lightingMemoryBarrier.image = outLightingAttachment[imageIndex].image;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &lightingMemoryBarrier);
	}

	VkImageView LightingPass::getLightingAttachment(int imageIndex)
	{
		return outLightingAttachment[imageIndex].imageView;
	}

	void LightingPass::recreateSwapchain(std::shared_ptr<VtSwapChain> newSwapchain)
	{
		this->swapchain = newSwapchain;
		vtPipeline.reset(nullptr);
		vkDestroySampler(device.device(), gBufferSampler, nullptr);
		cleanAttachments();
		cleanFramebuffer();

		createAttachments();
		createFramebuffer();
		createDefaultPipeline();
		createPipelineRessources();
	}

	void LightingPass::createGBufferTexturesDescriptorSet()
	{
		gBufferTexturesDescriptorPool = VtDescriptorPool::Builder(device)
			.setMaxSets(swapchain->imageCount())
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4 * swapchain->imageCount())
			.build();

		gBufferTexturesDescriptorSetLayout = VtDescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();


		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST; //No filtering to draw the texture quad
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;


		vkCreateSampler(device.device(), &samplerInfo, nullptr, &gBufferSampler);

		gBufferTexturesDescriptorSets.resize(swapchain->imageCount());
		for (int i = 0; i < swapchain->imageCount(); i++)
		{
			VkDescriptorImageInfo albedoImageInfo = {};
			albedoImageInfo.sampler = gBufferSampler;
			albedoImageInfo.imageView = gBufferPass->getAlbedoAttachment(i);
			albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo positionImageInfo = {};
			positionImageInfo.sampler = gBufferSampler;
			positionImageInfo.imageView = gBufferPass->getPositionAttachment(i);
			positionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo normalImageInfo = {};
			normalImageInfo.sampler = gBufferSampler;
			normalImageInfo.imageView = gBufferPass->getNormalAttachment(i);
			normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VkDescriptorImageInfo depthImageInfo = {};
			depthImageInfo.sampler = gBufferSampler;
			depthImageInfo.imageView = gBufferPass->getDepthAttachment(i);
			depthImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VtDescriptorWriter(*gBufferTexturesDescriptorSetLayout, *gBufferTexturesDescriptorPool)
				.writeImage(0, &albedoImageInfo)
				.writeImage(1, &positionImageInfo)
				.writeImage(2, &normalImageInfo)
				.writeImage(3, &depthImageInfo)
				.build(gBufferTexturesDescriptorSets[i]);

		}
	}
}