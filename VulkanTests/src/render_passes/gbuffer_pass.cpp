#include "gbuffer_pass.hpp"
#include <array>

namespace vt
{
	GBufferPass::GBufferPass(VtDevice& deviceRef, std::shared_ptr<VtSwapChain> swapchainRef, std::vector<VkDescriptorSetLayout> descriptorSetLayouts) : VtRenderPass(deviceRef, swapchainRef)
	{
		createPipelineLayout(descriptorSetLayouts);
		createAttachments();
		createRenderPass();
		createFramebuffer();
		createDefaultPipeline();
	}

	GBufferPass::~GBufferPass()
	{
		//virtual desctructor ! VtRenderPassDestructor is called so no need to destroy framebuffer
		cleanAttachments();
	}

	void GBufferPass::cleanAttachments() {
		VkDevice vkDevice = device.device();
		for (size_t i = 0; i < swapchain->imageCount(); i++)
		{
			albedoRoughnessAttachments[i].cleanAttachment(vkDevice);
			positionAttachments[i].cleanAttachment(vkDevice);
			normalMetallicAttachments[i].cleanAttachment(vkDevice);
			depthAttachments[i].cleanAttachment(vkDevice);
		}
	}

	void GBufferPass::createRenderPass()
	{
		//Attachments Descriptions
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = depthAttachments[0].format;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


		VkAttachmentDescription albedoAttachment = {};
		albedoAttachment.format = albedoRoughnessAttachments[0].format;
		albedoAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		albedoAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		albedoAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		albedoAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		albedoAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		albedoAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		albedoAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription normalAttachment = {};
		normalAttachment.format = normalMetallicAttachments[0].format;
		normalAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		normalAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription positionAttachment = {};
		positionAttachment.format = positionAttachments[0].format;
		positionAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		positionAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		positionAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		positionAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		positionAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		positionAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		positionAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		std::array<VkAttachmentDescription, 4> attachmentDescs = { albedoAttachment, positionAttachment, normalAttachment, depthAttachment };


		//Attachment References
		std::vector<VkAttachmentReference> colorReferences;
		colorReferences.push_back({ 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		colorReferences.push_back({ 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		colorReferences.push_back({ 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });

		VkAttachmentReference depthReference = {};
		depthReference.attachment = 3;
		depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 3;
		subpass.pColorAttachments = colorReferences.data();
		subpass.pDepthStencilAttachment = &depthReference;

		//Dependencies
		std::array<VkSubpassDependency, 2> dependencies;
		//Waiting for previous frame to be completely finished before reading the attachments
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
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependencies.data();

		VK_CHECK_RESULT(vkCreateRenderPass(device.device(), &renderPassInfo, nullptr, &renderPass));


	}

	void GBufferPass::createFramebuffer()
	{
		framebuffers.resize(swapchain->imageCount());
		for (size_t i = 0; i < swapchain->imageCount(); i++)
		{
			std::array<VkImageView, 4> attachments = { albedoRoughnessAttachments[i].imageView, positionAttachments[i].imageView, normalMetallicAttachments[i].imageView, depthAttachments[i].imageView };

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

	void GBufferPass::createAttachments()
	{
		//Images
		VkExtent2D extent = swapchain->getSwapChainExtent();

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



		albedoRoughnessAttachments.resize(swapchain->imageCount());
		positionAttachments.resize(swapchain->imageCount());
		normalMetallicAttachments.resize(swapchain->imageCount());
		depthAttachments.resize(swapchain->imageCount());

		for (size_t i = 0; i < swapchain->imageCount(); i++)
		{
			imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT; //Sampled for further use as a texture
			positionAttachments[i].format = VK_FORMAT_R16G16B16A16_SFLOAT; //Higher precision for position and normal data
			normalMetallicAttachments[i].format = VK_FORMAT_R16G16B16A16_SFLOAT;
			imageInfo.format = positionAttachments[i].format;
			device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, positionAttachments[i].image, positionAttachments[i].deviceMemory);
			device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, normalMetallicAttachments[i].image, normalMetallicAttachments[i].deviceMemory);

			albedoRoughnessAttachments[i].format = VK_FORMAT_R8G8B8A8_UNORM; //Linear color until frame presenting pass
			imageInfo.format = albedoRoughnessAttachments[i].format;
			device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, albedoRoughnessAttachments[i].image, albedoRoughnessAttachments[i].deviceMemory);

			depthAttachments[i].format = swapchain->findDepthFormat();
			imageInfo.format = depthAttachments[i].format;
			imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthAttachments[i].image, depthAttachments[i].deviceMemory);
		}

		//Image Views
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		for (size_t i = 0; i < swapchain->imageCount(); i++)
		{
			viewInfo.image = albedoRoughnessAttachments[i].image;
			viewInfo.format = albedoRoughnessAttachments[i].format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			VK_CHECK_RESULT(vkCreateImageView(device.device(), &viewInfo, nullptr, &albedoRoughnessAttachments[i].imageView));

			viewInfo.image = positionAttachments[i].image;
			viewInfo.format = positionAttachments[i].format;
			VK_CHECK_RESULT(vkCreateImageView(device.device(), &viewInfo, nullptr, &positionAttachments[i].imageView));

			viewInfo.image = normalMetallicAttachments[i].image;
			viewInfo.format = normalMetallicAttachments[i].format;
			VK_CHECK_RESULT(vkCreateImageView(device.device(), &viewInfo, nullptr, &normalMetallicAttachments[i].imageView));

			viewInfo.image = depthAttachments[i].image;
			viewInfo.format = depthAttachments[i].format;
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			VK_CHECK_RESULT(vkCreateImageView(device.device(), &viewInfo, nullptr, &depthAttachments[i].imageView));
		}


	}

	void GBufferPass::createPipelineLayout(std::vector<VkDescriptorSetLayout> descriptorSetLayouts)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void GBufferPass::createDefaultPipeline()
	{
		PipelineConfigInfo pipelineConfig{};
		VtPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		pipelineConfig.attachmentCount = 3;

		vtPipeline = std::make_unique<VtPipeline>(
			device,
			G_BUFFER_PASS_VERTEX_SHADER_PATH,
			G_BUFFER_PASS_FRAGMENT_SHADER_PATH,
			pipelineConfig);
	}

	void GBufferPass::updatePipelineRessources()
	{
		//No render pass specific ressource to update (Uniform buffers, images...)
	}

	void GBufferPass::createPipelineRessources()
	{
		//No render pass specific ressource to create (Uniform buffers, images...)
	}

	void GBufferPass::startRenderPass(VkCommandBuffer commandBuffer, int currentImageIndex)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffers[currentImageIndex];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapchain->getSwapChainExtent();

		std::array<VkClearValue, 4> clearValues{};
		clearValues[0].color = { 0.f, 0.f, 0.f, 1.0f };
		clearValues[1].color = { 0.f, 0.f, 0.f, 1.0f };
		clearValues[2].color = { 0.f, 0.f, 0.f, 1.0f };
		clearValues[3].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

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
	}


	void GBufferPass::endRenderPass(VkCommandBuffer commandBuffer, int imageIndex)
	{

		vkCmdEndRenderPass(commandBuffer);

		//Transitionning layout of Gbuffer images for texture sampling purposes
		VkImageMemoryBarrier colorMemoryBarrier{};
		colorMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		colorMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		colorMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		colorMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		colorMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		colorMemoryBarrier.subresourceRange.layerCount = 1;
		colorMemoryBarrier.subresourceRange.baseMipLevel = 0;
		colorMemoryBarrier.subresourceRange.levelCount = 1;


		std::vector<VkImageMemoryBarrier> gBufferColorBarriers{};
		colorMemoryBarrier.image = albedoRoughnessAttachments[imageIndex].image;
		gBufferColorBarriers.push_back(colorMemoryBarrier);
		colorMemoryBarrier.image = positionAttachments[imageIndex].image;
		gBufferColorBarriers.push_back(colorMemoryBarrier);
		colorMemoryBarrier.image = normalMetallicAttachments[imageIndex].image;
		gBufferColorBarriers.push_back(colorMemoryBarrier);

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, gBufferColorBarriers.size(), gBufferColorBarriers.data());

		//Transitionning Depth needs other stage masks
		VkImageMemoryBarrier depthMemoryBarrier = colorMemoryBarrier;
		depthMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		depthMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		depthMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		depthMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		depthMemoryBarrier.image = depthAttachments[imageIndex].image;

		vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &depthMemoryBarrier);


	}

	VkImageView GBufferPass::getAlbedoAttachment(uint32_t imageIndex)
	{

		assert(imageIndex < swapchain->imageCount() && "imageIndex out of range");
		return albedoRoughnessAttachments[imageIndex].imageView;
	}

	VkImageView GBufferPass::getPositionAttachment(uint32_t imageIndex)
	{
		assert(imageIndex < swapchain->imageCount() && "imageIndex out of range");
		return positionAttachments[imageIndex].imageView;
	}

	VkImageView GBufferPass::getNormalAttachment(uint32_t imageIndex)
	{
		assert(imageIndex < swapchain->imageCount() && "imageIndex out of range");
		return normalMetallicAttachments[imageIndex].imageView;
	}

	VkImageView GBufferPass::getDepthAttachment(uint32_t imageIndex)
	{
		assert(imageIndex < swapchain->imageCount() && "imageIndex out of range");
		return depthAttachments[imageIndex].imageView;
	}

	void GBufferPass::recreateSwapchain(std::shared_ptr<VtSwapChain> newSwapchain)
	{
		swapchain = newSwapchain;
		vtPipeline.reset(nullptr);

		cleanAttachments();
		cleanFramebuffer();

		createAttachments();
		createFramebuffer();
		createDefaultPipeline();
	}
}