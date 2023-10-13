#pragma once

// #VKRay
#include "nvvk/debug_util_vk.hpp"
#include "nvvk/descriptorsets_vk.hpp"
#include "nvvk/memallocator_dma_vk.hpp"
#include "nvvk/resourceallocator_vk.hpp"
#include "host_device.h"
#include "nvvk/raytraceKHR_vk.hpp"

#include "vt_swap_chain.hpp"
#include "vt_game_object.hpp"

namespace vt {
	class RTSetup
	{
		RTSetup(VtModel& model, VtDevice& device, VtGameObject::Map& gameObjects, VtSwapChain& swapChain);
		~RTSetup();

		void initRayTracing();
		auto objectToVkGeometryKHR(VtModel& model);
		void createBottomLevelAS();
		void createTopLevelAS();
		void createRtDescriptorSet();
		void updateRtDescriptorSet();
		void createOffscreenRender();
		nvmath::mat4f convertGLMToNVMATH(const glm::mat4& glmMatrix);

		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		nvvk::RaytracingBuilderKHR                      m_rtBuilder;
		nvvk::DescriptorSetBindings                     m_rtDescSetLayoutBind;
		VkDescriptorPool                                m_rtDescPool;
		VkDescriptorSetLayout                           m_rtDescSetLayout;
		VkDescriptorSet                                 m_rtDescSet;

		VtSwapChain& swapChain;
		VtGameObject::Map& gameObjects;
		VtDevice& m_device;
		VtModel& m_model;

		nvvk::ResourceAllocatorDma m_alloc;
		nvvk::ResourceAllocatorDedicated d_malloc;
	};
}


