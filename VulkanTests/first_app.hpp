#pragma once

#include "vt_window.hpp"
#include "vt_pipeline.hpp"

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
		VtPipeline vtPipeline{ "shaders/simple_shader.vert.spv", "shaders/simple_shader.frag.spv" };
	};
}