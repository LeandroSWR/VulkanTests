#pragma once

#include "vt_descriptors.hpp"
#include "vt_device.hpp"
#include "vt_model.hpp"
#include "vt_game_object.hpp"
#include "vt_window.hpp"
#include "vt_renderer.hpp"
#include "vt_texture.hpp"

// std
#include <memory>
#include <vector>

namespace vt
{
	class FirstApp
	{
	public:
		static constexpr int WIDTH = 1024;
		static constexpr int HEIGHT = 768;

		FirstApp();
		~FirstApp();

		FirstApp(const FirstApp&) = delete;
		FirstApp &operator=(const FirstApp&) = delete;

		void run();

	private:
		void loadGameObjects();

		VtWindow vtWindow{ WIDTH , HEIGHT, "Hello Vulkan!" };
		VtDevice vtDevice{ vtWindow };
		VtRenderer vtRenderer{ vtWindow, vtDevice };

		// Order of declarations matters! :(
		std::unique_ptr<VtDescriptorPool> globalPool{};
		std::unique_ptr<Texture> texture{};
		VtGameObject::Map gameObjects;
	};
}