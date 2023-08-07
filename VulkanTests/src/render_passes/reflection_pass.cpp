#include "reflection_pass.hpp"
#include <array>

namespace vt {
	ReflectionPass::ReflectionPass(VtDevice& deviceRef, std::shared_ptr<VtSwapChain> swapchainRef, std::vector<VkDescriptorSetLayout> descriptorSetLayouts, std::shared_ptr<GBufferPass> gBufferPass, std::shared_ptr<LightingPass> lightingPass) : VtRenderPass(deviceRef, swapchainRef)
	{
		this->gBufferPass = gBufferPass;
		this->lightingPass = lightingPass;
		createGBufferTexturesDescriptorSet();
		createPipelineLayout(descriptorSetLayouts);
		createAttachments();
		createRenderPass();
		createFramebuffer();
		createDefaultPipeline();
	}

	ReflectionPass::~ReflectionPass()
	{
		cleanAttachments();
		vkDestroySampler(device.device(), gBufferSampler, nullptr);
	}

	void ReflectionPass::createRenderPass()
	{
		VkAttachmentDescription outLightingAttachmentDescription{};
		outLightingAttachmentDescription.format = swapchain->getSwapChainImageFormat();
		outLightingAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		outLightingAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; //No need to clear if we fill the screen
		outLightingAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		outLightingAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		outLightingAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		outLightingAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		outLightingAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		// New attachment for debug output
		VkAttachmentDescription debugAttachmentDescription{};
		debugAttachmentDescription.format = outReflectionDebugAttachment[0].format;
		debugAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		debugAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		debugAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		debugAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		debugAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		debugAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		debugAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		std::array<VkAttachmentDescription, 2> attachmentDescs = { 
			outLightingAttachmentDescription, 
			debugAttachmentDescription
		};

		//Attachment References
		std::vector<VkAttachmentReference> colorReferences;
		colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
		subpass.pColorAttachments = colorReferences.data();

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
		renderPassInfo.pAttachments = attachmentDescs.data();
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass));
	}

	void ReflectionPass::createFramebuffer()
	{
		framebuffers.resize(swapchain->imageCount());
		for (size_t i = 0; i < swapchain->imageCount(); i++)
		{
			std::vector<VkImageView> attachments = { 
				swapchain->getImageView(i),
				outReflectionDebugAttachment[i].imageView
			};

			VkExtent2D swapChainExtent = swapchain->getSwapChainExtent();
			VkFramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
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

	void ReflectionPass::createAttachments()
	{
		//Images
		VkExtent2D extent = swapchain->getSwapChainExtent();
		outReflectionAttachment.resize(swapchain->imageCount());
		outReflectionDebugAttachment.resize(swapchain->imageCount());

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = extent.width;
		imageInfo.extent.height = extent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;


		for (size_t i = 0; i < swapchain->imageCount(); i++) {
			outReflectionAttachment[i].format = swapchain->getSwapChainImageFormat();
			imageInfo.format = outReflectionAttachment[i].format;
			device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outReflectionAttachment[i].image, outReflectionAttachment[i].deviceMemory);

			outReflectionDebugAttachment[i].format = VK_FORMAT_R16G16B16A16_SFLOAT;
			imageInfo.format = outReflectionDebugAttachment[i].format;
			device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, outReflectionDebugAttachment[i].image, outReflectionDebugAttachment[i].deviceMemory);
		}

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		for (size_t i = 0; i < swapchain->imageCount(); i++) {
			viewInfo.image = outReflectionAttachment[i].image;
			viewInfo.format = outReflectionAttachment[i].format;
			VK_CHECK_RESULT(vkCreateImageView(device.device(), &viewInfo, nullptr, &outReflectionAttachment[i].imageView));

			viewInfo.image = outReflectionDebugAttachment[i].image;
			viewInfo.format = outReflectionDebugAttachment[i].format;
			VK_CHECK_RESULT(vkCreateImageView(device.device(), &viewInfo, nullptr, &outReflectionDebugAttachment[i].imageView));
		}

	}

	void ReflectionPass::cleanAttachments()
	{
		VkDevice vkDevice = device.device();
		for (size_t i = 0; i < swapchain->imageCount(); i++)
		{
			outReflectionAttachment[i].cleanAttachment(vkDevice);
			outReflectionDebugAttachment[i].cleanAttachment(vkDevice);
		}
	}

	void ReflectionPass::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts)
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

	void ReflectionPass::createDefaultPipeline()
	{
		PipelineConfigInfo pipelineConfig{};
		VtPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipelineConfig.attachmentCount = 2;
		pipelineConfig.depthStencilInfo.depthTestEnable = VK_FALSE;
		pipelineConfig.attributeDescriptions = {}; //No vertex attribute expected in the shader

		vtPipeline = std::make_unique<VtPipeline>(
			device,
			LIGHTING_PASS_VERTEX_SHADER_PATH,
			LIGHTING_PASS_FRAGMENT_SHADER_PATH,
			pipelineConfig);
	}


	void ReflectionPass::createPipelineRessources()
	{
		createGBufferTexturesDescriptorSet();
	}

	void ReflectionPass::startRenderPass(VkCommandBuffer commandBuffer, int currentImageIndex)
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
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, &gBufferTexturesDescriptorSets[currentImageIndex], 0, nullptr);
	}

	void ReflectionPass::endRenderPass(VkCommandBuffer commandBuffer, int imageIndex)
	{
		vkCmdEndRenderPass(commandBuffer);
	}

	void ReflectionPass::recreateSwapchain(std::shared_ptr<VtSwapChain> newSwapchain)
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

	void ReflectionPass::createGBufferTexturesDescriptorSet()
	{
		gBufferTexturesDescriptorPool = VtDescriptorPool::Builder(device)
			.setMaxSets(swapchain->imageCount())
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 5 * swapchain->imageCount())
			.build();

		gBufferTexturesDescriptorSetLayout = VtDescriptorSetLayout::Builder(device)
			.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();


		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST;
		samplerInfo.minFilter = VK_FILTER_NEAREST;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; //Black outside of UV [0;1] to avoid repetition effects creating erroneous ray marching results
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK; //Black will not be reflected due to a condition in the shader if (reflectionColor.rgb != vec3(0.f)){


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

			VkDescriptorImageInfo lightingImageInfo = {};
			lightingImageInfo.sampler = gBufferSampler;
			lightingImageInfo.imageView = lightingPass->getLightingAttachment(i);
			lightingImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			VtDescriptorWriter(*gBufferTexturesDescriptorSetLayout, *gBufferTexturesDescriptorPool)
				.writeImage(0, &lightingImageInfo)
				.writeImage(1, &albedoImageInfo)
				.writeImage(2, &positionImageInfo)
				.writeImage(3, &normalImageInfo)
				.writeImage(4, &depthImageInfo)
				.build(gBufferTexturesDescriptorSets[i]);

		}
	}
}