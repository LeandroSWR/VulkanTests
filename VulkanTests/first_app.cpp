#include "first_app.hpp"
#include "keyboard_movement_controller.hpp"
#include "vt_camera.hpp"
#include "simple_render_system.hpp"
#include "vt_buffer.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <chrono>
#include <stdexcept>

namespace vt
{
	struct GlobalUbo {
		glm::mat4 projectionView{ 1.f };
		glm::vec3 lightDirection = glm::normalize(glm::vec3{ 1.f, -3.f, -1.f });
	};

	FirstApp::FirstApp()
	{
		loadGameObjects();
	}

	FirstApp::~FirstApp() {}

	void FirstApp::run()
	{
		std::vector<std::unique_ptr<VtBuffer>> uboBuffers(VtSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < uboBuffers.size(); i++)
		{
			uboBuffers[i] = std::make_unique<VtBuffer>(
				vtDevice,
				sizeof(GlobalUbo),
				1,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			
			uboBuffers[i]->map();
		}

		SimpleRenderSystem simpleRenderSystem{ vtDevice, vtRenderer.getSwapChainRenderPass() };
        VtCamera camera{};
        camera.setViewTarget(glm::vec3(-1.f, -2.f, -2.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewerObject = VtGameObject::createGameObject();
        KeyboardMovementController cameraController{};

        auto currentTime = std::chrono::high_resolution_clock::now();

		while (!vtWindow.shouldClose())
		{
			glfwPollEvents();

            auto newTime = std::chrono::high_resolution_clock::now();
            float frameTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            cameraController.moveInPlaneXZ(vtWindow.getGLFWwindow(), frameTime, viewerObject);
            camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

            float aspect = vtRenderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);
			
			if (auto commandBuffer = vtRenderer.beginFrame())
			{
				int frameIndex = vtRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime,
					commandBuffer,
					camera
				};

				// update
				GlobalUbo ubo{};
				ubo.projectionView = camera.getProjection() * camera.getView();
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				vtRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
				vtRenderer.endSwapChainRenderPass(commandBuffer);
				vtRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(vtDevice.device());
	}

	void FirstApp::loadGameObjects()
	{
        std::shared_ptr<VtModel> vtModel = VtModel::createModelFromFile(vtDevice, "models/flat_vase.obj");

        auto flatVase = VtGameObject::createGameObject();
		flatVase.model = vtModel;
		flatVase.transform.translation = { -.5f, .5f, 2.5f };
		flatVase.transform.scale = { 3.f, 1.5f, 3.f };
        gameObjects.push_back(std::move(flatVase));

		vtModel = VtModel::createModelFromFile(vtDevice, "models/smooth_vase.obj");

		auto smoothVase = VtGameObject::createGameObject();
		smoothVase.model = vtModel;
		smoothVase.transform.translation = { .5f, .5f, 2.5f };
		smoothVase.transform.scale = { 3.f, 1.5f, 3.f };
		gameObjects.push_back(std::move(smoothVase));
	}
}