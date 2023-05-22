#pragma once

#include <vulkan/vulkan_core.h>
#include "vt_buffer.hpp"
#include "vt_device.hpp"
#include "vt_texture.hpp"
#include "vt_descriptors.hpp"
#include "vt_frame_info.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <memory>
#include <vector>
#include <filesystem>

namespace vt {
	class VtModel {
	public:
		struct PBRParameters
		{
			glm::vec4 base_color_factor = { 1.0, 1.0, 1.0, 1.0 };
			glm::vec3 emissive_factor = { 1.0, 1.0, 1.0 };
			float metallic_factor = 1.0;
			float roughness_factor = 1.0;
			float scale = 1.0;
			float strength = 1.0;
			float alpha_cut_off = 1.0;
			float alpha_mode = 1.0;

			int has_base_color_texture = 0;
			int has_metallic_roughness_texture = 0;
			int has_normal_texture = 0;
			int has_occlusion_texture = 0;
			int has_emissive_texture = 0;
		};

		struct PBRMaterial
		{
			std::shared_ptr<Texture> base_color_texture;
			std::shared_ptr<Texture> metallic_roughness_texture;
			std::shared_ptr<Texture> normal_texture;
			std::shared_ptr<Texture> occlusion_texture;
			std::shared_ptr<Texture> emissive_texture;
			PBRParameters pbr_parameters = {};

			//std::shared_ptr<VtBuffer> pbr_parameters_buffer = {};
			VkDescriptorSet descriptor_set = {};
		};

		struct Material
		{
			std::shared_ptr<Texture> albedoTexture;
			std::shared_ptr<Texture> normalTexture;
			std::shared_ptr<Texture> metallicRoughnessTexture;
			VkDescriptorSet descriptorSet;
		};

		struct Primitive
		{
			uint32_t firstIndex;
			uint32_t firstVertex;
			uint32_t indexCount;
			uint32_t vertexCount;
			PBRMaterial material;
		};
		
		struct Vertex {
			glm::vec3 position{};
			glm::vec3 normal{};
			glm::vec4 tangent{};
			glm::vec2 uv{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

			bool operator==(const Vertex& other) const
			{
				return position == other.position && normal == other.normal && uv == other.uv;
			}
		};

		VtModel(VtDevice& device, const std::string& filepath, VtDescriptorSetLayout& setLayout, VtDescriptorPool& pool);
		~VtModel();

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer, VkDescriptorSet globalDescriptorSet, VkPipelineLayout pipelineLayout);
		
	private:
		void createVertexBuffers(const std::vector<Vertex>& vertices);
		void createIndexBuffers(const std::vector<uint32_t>& indices);

		std::unique_ptr<VtBuffer> vertexBuffer;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		std::vector<Primitive> primitives;
		std::vector<std::shared_ptr<Texture>> images;

		bool hasIndexBuffer = false;
		std::unique_ptr<VtBuffer> indexBuffer;
		VtDevice& vtDevice;
	};
}