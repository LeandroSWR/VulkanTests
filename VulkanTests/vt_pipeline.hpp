#pragma once

#include "vt_device.hpp"

// std
#include <string>
#include <vector>

namespace vt {

	struct PipelineConfigInfo {
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		VkViewport viewport;
		VkRect2D scissor;
		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};


	class VtPipeline {
	public:
		VtPipeline(
			VtDevice &device, 
			const std::string& vertFilepath, 
			const std::string& fragFilepath, 
			const PipelineConfigInfo& configInfo);
		~VtPipeline();

		VtPipeline(const VtPipeline&) = delete;
		void operator=(const VtPipeline&) = delete;

		static void defaultPipelineConfigInfo(
			PipelineConfigInfo& configInfo, uint32_t width, uint32_t height);

	private:
		static std::vector<char> readFile(const std::string& filepath);

		void createGraphicsPipeline(
			const std::string& vertFilepath, 
			const std::string& fragFilepath, 
			const PipelineConfigInfo& configInfo);

		void createShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

		VtDevice& vtDevice;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule;
		VkShaderModule fragShaderModule;
	};
}