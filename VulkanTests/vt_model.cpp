#include "vt_model.hpp"

//libs
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

// std
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

namespace vt {
	VtModel::VtModel(VtDevice& device, const VtModel::Builder & builder) : vtDevice{device}
	{
		createVertexBuffer(builder.vertices);
		createIndexBuffer(builder.indices);
	}

	VtModel::~VtModel()
	{
		vkDestroyBuffer(vtDevice.device(), vertexBuffer, nullptr);
		vkFreeMemory(vtDevice.device(), vertexBufferMemory, nullptr);

		if (hasIndexBuffer)
		{
			vkDestroyBuffer(vtDevice.device(), indexBuffer, nullptr);
			vkFreeMemory(vtDevice.device(), indexBufferMemory, nullptr);
		}
	}

	std::unique_ptr<VtModel> VtModel::createModelFromFile(VtDevice& device, const std::string& filepath)
	{
		Builder builder{};
		builder.loadModel(filepath);

		std::cout << "Vertex count: " << builder.vertices.size() << "\n";

		return std::make_unique<VtModel>(device, builder);
	}

	void VtModel::createVertexBuffer(const std::vector<Vertex>& vertices)
	{
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3.");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		vtDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(vtDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(vtDevice.device(), stagingBufferMemory);

		vtDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			vertexBuffer,
			vertexBufferMemory);

		vtDevice.copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vkDestroyBuffer(vtDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(vtDevice.device(), stagingBufferMemory, nullptr);
	}

	void VtModel::createIndexBuffer(const std::vector<uint32_t>& indices)
	{
		indexCount = static_cast<uint32_t>(indices.size());
		hasIndexBuffer = indexCount > 0;

		if (!hasIndexBuffer)
			return;

		VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		vtDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(vtDevice.device(), stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(vtDevice.device(), stagingBufferMemory);

		vtDevice.createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			indexBuffer,
			indexBufferMemory);

		vtDevice.copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(vtDevice.device(), stagingBuffer, nullptr);
		vkFreeMemory(vtDevice.device(), stagingBufferMemory, nullptr);
	}

	void VtModel::draw(VkCommandBuffer commandBuffer)
	{
		if (hasIndexBuffer)
		{
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		}
		else
		{
			vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
		}
	}

	void VtModel::bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = { vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (hasIndexBuffer)
		{
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}
	}

	std::vector<VkVertexInputBindingDescription> VtModel::Vertex::getBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> VtModel::Vertex::getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) });
		attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) });
		attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
		attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });


		return attributeDescriptions;
	}

	void VtModel::Builder::loadModel(const std::string& filepath)
	{
		//// Load the mesh
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);

		// Check if any meshes where loaded
		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
		{
			throw std::runtime_error("Unable to load file.");
		}
		
		const aiMesh* mesh = scene->mMeshes[0];

		// process vertices
		for (size_t i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex{};

			aiVector3D vert = mesh->mVertices[i];
			vertex.position = glm::vec3(vert.x, vert.y, vert.z);

			aiVector3D norm = mesh->mNormals[i];
			vertex.normal = glm::vec3(norm.x, norm.y, norm.z);

			if (mesh->mTextureCoords[0])
			{
				vertex.uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			}
			else
			{
				vertex.uv = glm::vec2(0.0f, 0.0f);
			}

			// process vertex colors if they exist
			for (int j = 0; j < AI_MAX_NUMBER_OF_COLOR_SETS; j++) {
				if (mesh->HasVertexColors(j)) {
					vertex.color.r = mesh->mColors[j][i].r;
					vertex.color.g = mesh->mColors[j][i].g;
					vertex.color.b = mesh->mColors[j][i].b;
					break;
				}
				else 
				{
					vertex.color = { 1.f, 1.f, 1.f };
				}
			}

			vertices.push_back(vertex);
		}

		// process indices
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; j++)
			{
				indices.push_back(face.mIndices[j]);
			}
		}
	}
}