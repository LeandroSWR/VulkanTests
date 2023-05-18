#pragma once

#include <vulkan/vulkan_core.h>
#include "vt_buffer.hpp"
#include "vt_device.hpp"
#include "vt_texture.hpp"
#include "vt_descriptors.hpp"

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
			Material material;
		};

		struct Vertex {
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec4 tangent{};
			glm::vec2 uv{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

			bool operator==(const Vertex& other) const
			{
				return position == other.position && color == other.color && normal == other.normal && uv == other.uv;
			}
		};

		/*struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			void loadModel(const std::string& filepath);
		};*/

		//VtModel(VtDevice &device, const VtModel::Builder &builder);
		VtModel(VtDevice& device, const std::string& filepath, VtDescriptorSetLayout& setLayout, VtDescriptorPool& pool);
		~VtModel();

		VtModel(const VtModel&) = delete;
		VtModel& operator=(const VtModel&) = delete;

		//static std::unique_ptr<VtModel> createModelFromFile(VtDevice& device, const std::string& filepath);

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

		/*std::unique_ptr<VtBuffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<VtBuffer> indexBuffer;
		uint32_t indexCount;*/
	};
}