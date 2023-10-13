#include "first_app.hpp"
#include <vulkan/vulkan_core.h>
#include <memory>

#include "keyboard_movement_controller.hpp"
#include "vt_buffer.hpp"
#include "vt_camera.hpp"
#include "systems/point_light_system.hpp"
#include "systems/simple_render_system.hpp"
#include "RTSetup.hpp";

#include "nvpsystem.hpp"

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

//#define RENDER_INDICATORS

// Default search path for shaders
std::vector<std::string> defaultSearchPaths;

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
		// Search path for shaders and other media
		defaultSearchPaths = {
			NVPSystem::exePath() + "../../",
			NVPSystem::exePath() + "../../" "..",
			std::string("VulkanTests"),
		};

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
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
			.build();

		auto rtSetLayout = VtDescriptorSetLayout::Builder(vtDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR)
			.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
			.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR)
			.build();

		auto pbrMaterialSetLayout =
			VtDescriptorSetLayout::Builder(vtDevice)
			.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.addBinding(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
			.build();

		std::vector<VkDescriptorSetLayout> layouts = { globalSetLayout->getDescriptorSetLayout(), pbrMaterialSetLayout->getDescriptorSetLayout() };

		//Initializing render passes
		gBufferPass = std::make_shared<GBufferPass>(vtDevice, vtRenderer.getSwapchain(), layouts);
		lightingPass = std::make_shared<LightingPass>(vtDevice, vtRenderer.getSwapchain(), layouts, gBufferPass);
		reflectionPass = std::make_shared<ReflectionPass>(vtDevice, vtRenderer.getSwapchain(), layouts, gBufferPass, lightingPass);

		std::vector<VkDescriptorSet> globalDescriptorSets(VtSwapChain::MAX_FRAMES_IN_FLIGHT);
		for (int i = 0; i < globalDescriptorSets.size(); i++)
		{
			auto bufferInfo = uboBuffers[i]->getDescriptorInfo();
			VtDescriptorWriter(*globalSetLayout, *globalPool)
				.writeBuffer(0, &bufferInfo)
				.build(globalDescriptorSets[i]);

		}
		
		PointLightSystem pointLightSystem{
			vtDevice,
			gBufferPass->getRenderPass(), // Workaround to make the light position indicator appear. A better way would be to add another render pass and write indicators on top of the final image.
			globalSetLayout->getDescriptorSetLayout()
		};

		VtModelManager modelManager(vtDevice, "models/Sponza/Sponza.gltf", *pbrMaterialSetLayout, *globalPool);
		auto vtModels = modelManager.getModels();

		for (const auto& vtModel : vtModels)
		{
			auto gameObject = VtGameObject::createGameObject();
			gameObject.model = vtModel;
			gameObject.transform.translation = { 0.f, 0.f, 0.f };
			gameObject.transform.scale = { .01f, .01f, .01f };
			gameObject.transform.rotation = { 0.0f, 0.0f, 0.0f };// 3.14159265f};
			gameObjects.emplace(gameObject.getId(), std::move(gameObject));
		}

        VtCamera camera{};

        //camera.setViewTarget(glm::vec3(-1.f, -2.f, -2.f), glm::vec3(0.f, 0.f, 2.5f));
		camera.setViewTarget(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f));

        auto viewerObject = VtGameObject::createGameObject();
		viewerObject.transform.translation.z = -2.5f;
        KeyboardMovementController cameraController{};

        auto currentTime = std::chrono::high_resolution_clock::now();

		// Create example
		RTSetup helloVk(vtDevice, gameObjects, *vtRenderer.getSwapchain());

		// #VKRAY
		helloVk.initRayTracing();
		helloVk.createBottomLevelAS();
		//helloVk.createTopLevelAS();
		//helloVk.createRtDescriptorSet();
		//helloVk.createRtPipeline();
		//helloVk.createRtShaderBindingTable();

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
				int imageIndex = vtRenderer.getImageIndex();
				FrameInfo frameInfo{
					frameIndex,
					imageIndex,
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
				ubo.inverseView = camera.getInverseView();
				ubo.inverseProjection = camera.getInverseProjection();
				pointLightSystem.update(frameInfo, ubo);
				uboBuffers[frameIndex]->writeToBuffer(&ubo);
				uboBuffers[frameIndex]->flush();

				// render
				gBufferPass->startRenderPass(commandBuffer, imageIndex);
				gBufferPass->bindDefaultPipeline(commandBuffer);

				//Game object rendering //This drawing step could be abstracted as an abstract member function in VtRenderPass
				for (auto& kv : frameInfo.gameObjects)
				{
					auto& obj = kv.second;
					if (obj.model == nullptr) continue;

					SimplePushConstantData push{};
					push.modelMatrix = obj.transform.mat4();
					push.normalMatrix = obj.transform.normalMatrix();
					
					vkCmdPushConstants(
						frameInfo.commandBuffer,
						gBufferPass->getPipelineLayout(),
						VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
						0,
						sizeof(SimplePushConstantData),
						&push);
					obj.model->bind(frameInfo.commandBuffer);
					obj.model->draw(frameInfo.commandBuffer, frameInfo.globalDescriptorSet, gBufferPass->getPipelineLayout());
				}

#ifdef RENDER_INDICATORS

				pointLightSystem.render(frameInfo);
#endif

				gBufferPass->endRenderPass(commandBuffer, imageIndex);

				lightingPass->startRenderPass(commandBuffer, imageIndex);
				lightingPass->bindDefaultPipeline(commandBuffer);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, lightingPass->getPipelineLayout(), 0,
					1, &frameInfo.globalDescriptorSet, 0, nullptr);
				vkCmdDraw(commandBuffer, 6, 1, 0, 0); //Drawing the lit Texture
				lightingPass->endRenderPass(commandBuffer, imageIndex);

				reflectionPass->startRenderPass(commandBuffer, imageIndex);
				reflectionPass->bindDefaultPipeline(commandBuffer);
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, reflectionPass->getPipelineLayout(), 0,
					1, &frameInfo.globalDescriptorSet, 0, nullptr);
				vkCmdDraw(commandBuffer, 6, 1, 0, 0); //Drawing the lit Texture + reflections
				reflectionPass->endRenderPass(commandBuffer, imageIndex);

				//Recreating swapchain sized objects
				if (!vtRenderer.endFrame())
				{
					gBufferPass->recreateSwapchain(vtRenderer.getSwapchain());

					lightingPass->recreateSwapchain(vtRenderer.getSwapchain());

					reflectionPass->recreateSwapchain(vtRenderer.getSwapchain());
				}
			}
		}

		vkDeviceWaitIdle(vtDevice.device());
	}

	void FirstApp::loadGameObjects()
	{
		//std::shared_ptr<VtModel> vtModel = VtModel::createModelFromFile(vtDevice, "models/normal_cube.fbx");

        /*auto vikingRoom = VtGameObject::createGameObject();
		vikingRoom.model = vtModel;
		vikingRoom.transform.rotation = { 1.57f, 0.f, -1.57f };
		vikingRoom.transform.translation = { .0f, 0.5f, 0.f };
		vikingRoom.transform.scale = { 1.5f, 1.5f, 1.5f };
        gameObjects.emplace(vikingRoom.getId(), std::move(vikingRoom));*/

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
			auto pointLight = VtGameObject::makePointLight(10.f);
			pointLight.color = lightColors[i];
			auto rotateLight = glm::rotate(
				glm::mat4(0.8f),
				(i * glm::two_pi<float>()) / lightColors.size(),
				{ 0.f, -1.f, 0.f });
			pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(-1.f, 1.0f, -1.f, 1.f));
			gameObjects.emplace(pointLight.getId(), std::move(pointLight));
		}

		/*auto sunPointLight = VtGameObject::makePointLight(10000000.f, 10000.f);
		sunPointLight.transform.translation = glm::vec3(0.f, 30.f, 0.0);
		gameObjects.emplace(sunPointLight.getId(), std::move(sunPointLight));*/
	}
}