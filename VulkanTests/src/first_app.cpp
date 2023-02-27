#include "first_app.hpp"
#include "keyboard_movement_controller.hpp"
#include "vt_camera.hpp"
#include "vt_buffer.hpp"
#include "systems/simple_render_system.hpp"
#include "systems/point_light_system.hpp"

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
	FirstApp::FirstApp()
	{
		globalPool = VtDescriptorPool::Builder(vtDevice)
			.setMaxSets(VtSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VtSwapChain::MAX_FRAMES_IN_FLIGHT)
			.build();

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

		auto globalSetLayout = VtDescriptorSetLayout::Builder(vtDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(VtSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			VtDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{ 
			vtDevice, 
			vtRenderer.getSwapChainRenderPass(), 
			globalSetLayout->getDescriptorSetLayout()
		};
		PointLightSystem pointLightSystem{
			vtDevice,
			vtRenderer.getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()
		};
        VtCamera camera{};

        camera.setViewTarget(glm::vec3(-1.f, -2.f, -2.f), glm::vec3(0.f, 0.f, 2.5f));

        auto viewerObject = VtGameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
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
            camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 1000.f);
			
			if (auto commandBuffer = vtRenderer.beginFrame())
			{
				int frameIndex = vtRenderer.getFrameIndex();
				FrameInfo frameInfo{
					frameIndex,
					frameTime, 
					commandBuffer,
					camera,
					globalDescriptorSets[frameIndex],
					gameObjects
				};

				// update
				GlobalUbo ubo{};
				ubo.projection = camera.getProjection();
				ubo.view = camera.getView();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				vtRenderer.beginSwapChainRenderPass(commandBuffer);
				simpleRenderSystem.renderGameObjects(frameInfo);
				pointLightSystem.render(frameInfo);
				vtRenderer.endSwapChainRenderPass(commandBuffer);
				vtRenderer.endFrame();
			}
		}

		vkDeviceWaitIdle(vtDevice.device());
	}

	void FirstApp::loadGameObjects()
	{
        std::shared_ptr<VtModel> vtModel = VtModel::createModelFromFile(vtDevice, "models/flat_sphere.obj");

        auto flatSphere = VtGameObject::createGameObject();
		flatSphere.model = vtModel;
		flatSphere.transform.translation = { -.5f, .5f, 0.f };
		flatSphere.transform.scale = { 1.5f, 1.5f, 1.5f };
        gameObjects.emplace(flatSphere.getId(), std::move(flatSphere));

		vtModel = VtModel::createModelFromFile(vtDevice, "models/smooth_sphere.obj");
		auto smoothSphere = VtGameObject::createGameObject();
		smoothSphere.model = vtModel;
		smoothSphere.transform.translation = { .5f, .5f, 0.f };
		smoothSphere.transform.scale = { 1.5f, 1.5f, 1.5f };
		gameObjects.emplace(smoothSphere.getId(), std::move(smoothSphere));

		vtModel = VtModel::createModelFromFile(vtDevice, "models/quad.obj");
		auto floor = VtGameObject::createGameObject();
		floor.model = vtModel;
		floor.transform.translation = { 0.f, .5f, 0.f };
		floor.transform.scale = { 3.f, 1.f, 3.f };
		gameObjects.emplace(floor.getId(), std::move(floor));

		// Point Lights!
		std::vector<glm::vec3> lightColors{
			{1.f, .1f, .1f},
			{.1f, .1f, 1.f},
			{.1f, 1.f, .1f},
			{1.f, 1.f, .1f},
			{.1f, 1.f, 1.f},
			{1.f, 1.f, 1.f}
		};

		for (int i = 0; i < lightColors.size(); i++)
		{
			auto pointLight = VtGameObject::makePointLight(0.2f);
			pointLight.color = lightColors[i];
			auto rotateLight = glm::rotate(
				glm::mat4(1.f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{ 0.f, -1.f, 0.f });
			pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}
	}
}