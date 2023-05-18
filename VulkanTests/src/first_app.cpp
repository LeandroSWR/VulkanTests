#include "first_app.hpp"
#include <vulkan/vulkan_core.h>
#include <memory>

#include "keyboard_movement_controller.hpp"
#include "vt_buffer.hpp"
#include "vt_camera.hpp"
#include "systems/point_light_system.hpp"
#include "systems/simple_render_system.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <array>
#include <cassert>
#include <chrono>
#include <stdexcept>

namespace vt
{
	FirstApp::FirstApp()
	{
		globalPool = VtDescriptorPool::Builder(vtDevice)
			.setMaxSets(1000)
			.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VtSwapChain::MAX_FRAMES_IN_FLIGHT)
			.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000)
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

		texture = std::make_unique<Texture>(vtDevice, "textures/normal_cube_lambert1_BaseColor.png");

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.sampler = texture->getSampler();
		imageInfo.imageView = texture->getImageView();
		imageInfo.imageLayout = texture->getImageLayout();

		//// Load the specular texture
		//auto specularTexture = std::make_unique<Texture>(vtDevice, "textures/normal_cube_lambert1_Metallic.png");

		//VkDescriptorImageInfo specularInfo = {};
		//specularInfo.sampler = specularTexture->getSampler();
		//specularInfo.imageView = specularTexture->getImageView();
		//specularInfo.imageLayout = specularTexture->getImageLayout();

		//// Load the normal texture
		//auto normalTexture = std::make_unique<Texture>(vtDevice, "textures/normal_cube_lambert1_Normal.png");

		//VkDescriptorImageInfo normalInfo = {};
		//normalInfo.sampler = normalTexture->getSampler();
		//normalInfo.imageView = normalTexture->getImageView();
		//normalInfo.imageLayout = normalTexture->getImageLayout();

		auto globalSetLayout = VtDescriptorSetLayout::Builder(vtDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		auto materialSetLayout =
			VtDescriptorSetLayout::Builder(vtDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		std::vector<VkDescriptorSet> globalDescriptorSets(VtSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = uboBuffers[i]->descriptorInfo();
			VtDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.writeImage(1, &imageInfo)
				/*.writeImage(2, &specularInfo)
				.writeImage(3, &normalInfo)*/
				.build(globalDescriptorSets[i]);
		}

		SimpleRenderSystem simpleRenderSystem{ 
			vtDevice, 
			vtRenderer.getSwapChainRenderPass(), 
			{globalSetLayout->getDescriptorSetLayout(), materialSetLayout->getDescriptorSetLayout()}
		};
		PointLightSystem pointLightSystem{
			vtDevice,
			vtRenderer.getSwapChainRenderPass(),
			globalSetLayout->getDescriptorSetLayout()
		};

		std::shared_ptr<VtModel> lveModel = std::make_shared<VtModel>(vtDevice, "models/Sponza/Sponza.gltf", *materialSetLayout, *globalPool);
		auto floor = VtGameObject::createGameObject();
		floor.model = lveModel;
		floor.transform.translation = { 0.f, .5f, 0.f };
		floor.transform.scale = { .01f, .01f, .01f };
		floor.transform.rotation = { 0.0f, 0.0f, 3.14159265f };
		gameObjects.emplace(floor.getId(), std::move(floor));

        VtCamera camera{};

        camera.setViewTarget(glm::vec3(-1.f, -2.f, -2.f), glm::vec3(0.f, 0.f, 2.5f));
		//camera.setViewTarget(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f));

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
			camera.setView(viewerObject.transform.mat4());

            float aspect = vtRenderer.getAspectRatio();
            camera.setPerspectiveProjection(glm::radians(50.f), WIDTH, HEIGHT, 0.1f, 1000.f);
			
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
				ubo.iverseView = camera.getInverseView();
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
  //      std::shared_ptr<VtModel> vtModel = VtModel::createModelFromFile(vtDevice, "models/normal_cube.fbx");

  //      /*auto vikingRoom = VtGameObject::createGameObject();
		//vikingRoom.model = vtModel;
		//vikingRoom.transform.rotation = { 1.57f, 0.f, -1.57f };
		//vikingRoom.transform.translation = { .0f, 0.5f, 0.f };
		//vikingRoom.transform.scale = { 1.5f, 1.5f, 1.5f };
  //      gameObjects.emplace(vikingRoom.getId(), std::move(vikingRoom));*/

		//auto vikingRoom = VtGameObject::createGameObject();
		//vikingRoom.model = vtModel;
		//vikingRoom.transform.rotation = { 0.f, 0.f, 0.f };
		//vikingRoom.transform.translation = { .0f, 0.f, 0.f };
		//vikingRoom.transform.scale = { 0.01f, 0.01f, 0.01f };
		//gameObjects.emplace(vikingRoom.getId(), std::move(vikingRoom));

		//vtModel = VtModel::createModelFromFile(vtDevice, "models/plane_2.obj");
		//auto floor = VtGameObject::createGameObject();
		//floor.model = vtModel;
		//floor.transform.translation = { 0.f, -.5f, 0.f };
		//floor.transform.scale = { 5.f, 1.f, 5.f };
		//gameObjects.emplace(floor.getId(), std::move(floor));

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
			pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, -0.02f, -1.f, 1.f));
			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}
	}
}