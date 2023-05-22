#include "simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <stdexcept>

namespace vt
{

	struct SimplePushConstantData
	{
		glm::mat4 modelMatrix{ 1.0f };
		glm::mat4 normalMatrix{ 1.0f };
	};

	SimpleRenderSystem::SimpleRenderSystem(VtDevice& device, VkRenderPass renderPass, std::vector<VkDescriptorSetLayout> setLayouts) : vtDevice{device}
	{
		createPipelineLayout(setLayouts);
		createPipeline(renderPass);
	}

	SimpleRenderSystem::~SimpleRenderSystem()
	{
		vkDestroyPipelineLayout(vtDevice.device(), pipelineLayout, nullptr);
	}

	void SimpleRenderSystem::createPipelineLayout(std::vector<VkDescriptorSetLayout> setLayouts)
	{
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(SimplePushConstantData);

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutInfo.pSetLayouts = setLayouts.data();
		pipelineLayoutInfo.pushConstantRangeCount = 1;
		pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		if (vkCreatePipelineLayout(vtDevice.device(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}

	void SimpleRenderSystem::createPipeline(VkRenderPass renderPass)
	{
		assert(pipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

		PipelineConfigInfo pipelineConfig{};
		VtPipeline::defaultPipelineConfigInfo(pipelineConfig);
		pipelineConfig.renderPass = renderPass;
		pipelineConfig.pipelineLayout = pipelineLayout;
		vtPipeline = std::make_unique<VtPipeline>(
			vtDevice,
			"shaders/simple_shader.vert.spv",
			"shaders/simple_shader.frag.spv",
			pipelineConfig);
	}

	void SimpleRenderSystem::renderGameObjects(FrameInfo& frameInfo)
	{
		vtPipeline->bind(frameInfo.commandBuffer);

		/*vkCmdBindDescriptorSets(
			frameInfo.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&frameInfo.globalDescriptorSet,
			0,
			nullptr);*/

		for (auto& kv : frameInfo.gameObjects)
		{
			auto& obj = kv.second;
			if (obj.model == nullptr) continue;

			SimplePushConstantData push{};
			push.modelMatrix = obj.transform.mat4();
			push.normalMatrix = obj.transform.normalMatrix();

			vkCmdPushConstants(
				frameInfo.commandBuffer,
				pipelineLayout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				sizeof(SimplePushConstantData),
				&push);
			//obj.model->bind(frameInfo.commandBuffer);
			//obj.model->draw(frameInfo.commandBuffer, frameInfo.globalDescriptorSet, pipelineLayout);
		}
	}
}