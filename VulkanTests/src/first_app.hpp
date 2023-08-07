#pragma once

#include "vt_descriptors.hpp"
#include "vt_device.hpp"
#include "vt_model.hpp"
#include "vt_game_object.hpp"
#include "vt_window.hpp"
#include "vt_renderer.hpp"
#include "vt_texture.hpp"
#include "render_passes\gbuffer_pass.hpp"
#include "render_passes\lighting_pass.hpp"
#include "render_passes\reflection_pass.hpp"

// std
#include <memory>
#include <vector>

namespace vt
{
	class FirstApp
	{
	public:
		static constexpr int WIDTH = 1920;
		static constexpr int HEIGHT = 1080;

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
		std::shared_ptr<GBufferPass> gBufferPass;
		std::shared_ptr<LightingPass> lightingPass;
		std::shared_ptr<ReflectionPass> reflectionPass;
	};
}