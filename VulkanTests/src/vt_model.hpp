#pragma once

#include "vt_buffer.hpp"
#include "vt_device.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <memory>
#include <vector>

namespace vt {
	class VtModel {
	public:
		struct Vertex {
			glm::vec3 position{};
			glm::vec3 color{};
			glm::vec3 normal{};
			glm::vec2 uv{};

			static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
		};

		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			void loadModel(const std::string& filepath);
		};

		VtModel(VtDevice &device, const VtModel::Builder &builder);
		~VtModel();

		VtModel(const VtModel&) = delete;
		VtModel& operator=(const VtModel&) = delete;

		static std::unique_ptr<VtModel> createModelFromFile(VtDevice& device, const std::string& filepath);

		void bind(VkCommandBuffer commandBuffer);
		void draw(VkCommandBuffer commandBuffer);

	private:
		void createVertexBuffer(const std::vector<Vertex>& vertices);
		void createIndexBuffer(const std::vector<uint32_t>& indices);

		VtDevice& vtDevice;

		std::unique_ptr<VtBuffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<VtBuffer> indexBuffer;
		uint32_t indexCount;
	};
}