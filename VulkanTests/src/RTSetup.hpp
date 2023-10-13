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
	public:
		RTSetup(VtDevice& device, VtGameObject::Map& gameObjects, VtSwapChain& swapChain);
		~RTSetup();

		void initRayTracing();
		auto objectToVkGeometryKHR(VtModel& model);
		void createBottomLevelAS();
		void createTopLevelAS();
		void createRtDescriptorSet();
		void updateRtDescriptorSet();
		void createRtPipeline();
		void createRtShaderBindingTable();
		void raytrace(const VkCommandBuffer& cmdBuf, const nvmath::vec4f& clearColor);

	private:

		nvmath::mat4f convertGLMToNVMATH(const glm::mat4& glmMatrix);

		VkPhysicalDeviceRayTracingPipelinePropertiesKHR m_rtProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR };
		nvvk::RaytracingBuilderKHR                      m_rtBuilder;

		// RT Descriptors
		nvvk::DescriptorSetBindings                     m_rtDescSetLayoutBind;
		VkDescriptorPool                                m_rtDescPool;
		VkDescriptorSetLayout                           m_rtDescSetLayout;
		VkDescriptorSet                                 m_rtDescSet;

		// RT Pipeline
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> m_rtShaderGroups;
		VkPipelineLayout                                  m_rtPipelineLayout;
		VkPipeline                                        m_rtPipeline;

		// SBT Buffer
		nvvk::Buffer                    m_rtSBTBuffer;
		VkStridedDeviceAddressRegionKHR m_rgenRegion{};
		VkStridedDeviceAddressRegionKHR m_missRegion{};
		VkStridedDeviceAddressRegionKHR m_hitRegion{};
		VkStridedDeviceAddressRegionKHR m_callRegion{};

		VtDevice& m_device;
		VtSwapChain& swapChain;
		VtGameObject::Map& gameObjects;
		nvvk::DescriptorSetBindings m_descSetLayoutBind;
		VkDescriptorSetLayout m_descSetLayout;
		VkDescriptorSet m_descSet;
		
		// Information pushed at each draw call
		PushConstantRaster m_pcRaster{
			{1},                // Identity matrix
			{10.f, 15.f, 8.f},  // light position
			0,                  // instance Id
			100.f,              // light intensity
			0                   // light type
		};

		// Push constant for ray tracer
		PushConstantRay m_pcRay{ {}, {}, 0, 0, 10 };

		nvvk::ResourceAllocatorDma m_alloc;
		nvvk::DebugUtil            m_debug;
	};
}


