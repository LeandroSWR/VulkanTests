#pragma once

#include "vt_window.hpp"
#include "vt_pipeline.hpp"
#include "vt_device.hpp"

namespace vt
{
	class FirstApp
	{
	public:
		static constexpr int WIDTH = 800;
		static constexpr int HEIGHT = 600;

		void run();

	private:
		VtWindow vtWindow{ WIDTH , HEIGHT, "Hello Vulkan!" };
		VtDevice vtDevice{ vtWindow };
		VtPipeline vtPipeline{ 
			vtDevice, 
			"shaders/simple_shader.vert.spv", 
			"shaders/simple_shader.frag.spv", 
			VtPipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
	};
}