#pragma once

#include "vt_device.hpp"

// std
#include <string>
#include <vector>

namespace vt {

	struct PipelineConfigInfo {};
	class VtPipeline {
	public:
		VtPipeline(
			VtDevice &device, 
			const std::string& vertFilepath, 
			const std::string& fragFilepath, 
			const PipelineConfigInfo& configInfo);
		~VtPipeline() {}

		VtPipeline(const VtPipeline&) = delete;
		void operator=(const VtPipeline&) = delete;

		static PipelineConfigInfo defaultPipelineConfigInfo(uint32_t width, uint32_t height);

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