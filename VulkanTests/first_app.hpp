#pragma once

#include "vt_window.hpp"

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
	};
}