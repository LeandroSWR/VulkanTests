#include "vt_model.hpp"

#include "vt_descriptors.hpp"
#include "vt_device.hpp"
#include "vt_utils.hpp"

// libs
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/type_ptr.hpp>

// std
#include <cassert>
#include <cstring>
#include <unordered_map>
#include <filesystem>
#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define STBI_MSC_SECURE_CRT
#include <tiny_gltf.h>

namespace std
{
    template <>
    struct hash<vt::VtModel::Vertex>
    {
        size_t operator()(vt::VtModel::Vertex const& vertex) const
        {
            size_t seed = 0;
            vt::hashCombine(seed, vertex.position, vertex.normal, vertex.uv);
            return seed;
        }
    };
}  // namespace std

namespace vt
{
    void VtModel::createVertexBuffers(const std::vector<Vertex>& vertices)
    {
        uint32_t vertexCount = static_cast<uint32_t>(vertices.size());
        assert(vertexCount >= 3 && "Vertex count must be at least 3");
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
        uint32_t vertexSize = sizeof(vertices[0]);

        VtBuffer stagingBuffer{
            vtDevice,
            vertexSize,
            vertexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void*)vertices.data());

        vertexBuffer = std::make_unique<VtBuffer>(
            vtDevice,
            vertexSize,
            vertexCount,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vtDevice.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
    }

    void VtModel::createIndexBuffers(const std::vector<uint32_t>& indices)
    {
        uint32_t indexCount = static_cast<uint32_t>(indices.size());
        hasIndexBuffer = indexCount > 0;

        if (!hasIndexBuffer)
        {
            return;
        }

        VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
        uint32_t indexSize = sizeof(indices[0]);

        VtBuffer stagingBuffer{
            vtDevice,
            indexSize,
            indexCount,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        };

        stagingBuffer.map();
        stagingBuffer.writeToBuffer((void*)indices.data());

        indexBuffer = std::make_unique<VtBuffer>(
            vtDevice,
            indexSize,
            indexCount,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vtDevice.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
    }

    void VtModel::draw(VkCommandBuffer commandBuffer, VkDescriptorSet globalDescriptorSet, VkPipelineLayout pipelineLayout)
    {
        for (auto& primitive : primitives)
        {
            if (hasIndexBuffer)
            {
                std::vector<VkDescriptorSet> sets{ globalDescriptorSet, primitive.material.descriptor_set };
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0,
                    sets.size(), sets.data(), 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, primitive.firstVertex, 0);
            }
            else
            {
                vkCmdDraw(commandBuffer, primitive.vertexCount, 1, 0, 0);
            }
        }
    }

    void VtModel::bind(VkCommandBuffer commandBuffer)
    {
        VkBuffer buffers[] = { vertexBuffer->getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

        if (hasIndexBuffer)
        {
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32);
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
        attributeDescriptions.push_back({ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
        attributeDescriptions.push_back({ 2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent) });
        attributeDescriptions.push_back({ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });

        return attributeDescriptions;
    }

    VtModel::~VtModel() {}

    VtModel::VtModel(VtDevice& device, const std::string& filepath, VtDescriptorSetLayout& materialSetLayout, VtDescriptorPool& descriptorPool) : vtDevice{ device }
    {
        std::string warn, err;
        tinygltf::TinyGLTF GltfLoader;
        tinygltf::Model GltfModel;
        if (!GltfLoader.LoadASCIIFromFile(&GltfModel, &err, &warn, filepath))
        {
            throw std::runtime_error("failed to load gltf file!");
        }

        auto path = std::filesystem::path{ filepath };
        for (auto& image : GltfModel.images)
        {
            images.push_back(std::make_shared<Texture>(device, path.parent_path().append(image.uri).generic_string()));
        }

        for (auto& scene : GltfModel.scenes)
        {
            for (size_t i = 0; i < scene.nodes.size(); i++)
            {
                auto& node = GltfModel.nodes[i];
                uint32_t vertexOffset = 0;
                uint32_t indexOffset = 0;
                for (auto& GltfPrimitive : GltfModel.meshes[node.mesh].primitives)
                {
                    uint32_t vertexCount = 0;
                    uint32_t indexCount = 0;
                    const float* positionBuffer = nullptr;
                    const float* normalsBuffer = nullptr;
                    const float* texCoordsBuffer = nullptr;
                    const float* tangentsBuffer = nullptr;
                    if (GltfPrimitive.attributes.find("POSITION") != GltfPrimitive.attributes.end())
                    {
                        const tinygltf::Accessor& accessor = GltfModel.accessors[GltfPrimitive.attributes.find("POSITION")->second];
                        const tinygltf::BufferView& view = GltfModel.bufferViews[accessor.bufferView];
                        positionBuffer = reinterpret_cast<const float*>(&(GltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                        vertexCount = accessor.count;
                    }
                    if (GltfPrimitive.attributes.find("NORMAL") != GltfPrimitive.attributes.end())
                    {
                        const tinygltf::Accessor& accessor = GltfModel.accessors[GltfPrimitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& view = GltfModel.bufferViews[accessor.bufferView];
                        normalsBuffer = reinterpret_cast<const float*>(&(GltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    }
                    if (GltfPrimitive.attributes.find("TEXCOORD_0") != GltfPrimitive.attributes.end())
                    {
                        const tinygltf::Accessor& accessor = GltfModel.accessors[GltfPrimitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& view = GltfModel.bufferViews[accessor.bufferView];
                        texCoordsBuffer = reinterpret_cast<const float*>(&(GltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    }
                    if (GltfPrimitive.attributes.find("TANGENT") != GltfPrimitive.attributes.end())
                    {
                        const tinygltf::Accessor& accessor = GltfModel.accessors[GltfPrimitive.attributes.find("TANGENT")->second];
                        const tinygltf::BufferView& view = GltfModel.bufferViews[accessor.bufferView];
                        tangentsBuffer = reinterpret_cast<const float*>(&(GltfModel.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                    }

                    for (size_t i = 0; i < vertexCount; i++)
                    {
                        Vertex vertex{};
                        vertex.position = glm::make_vec3(&positionBuffer[i * 3]);
                        vertex.normal = glm::normalize(
                            glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[i * 3]) : glm::vec3(0.0f)));
                        vertex.tangent = glm::vec4(
                            tangentsBuffer ? glm::make_vec4(&tangentsBuffer[i * 4]) : glm::vec4(0.0f));;
                        vertex.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[i * 2]) : glm::vec2(0.0f);
                        vertices.push_back(vertex);
                    }

                    const tinygltf::Accessor& accessor = GltfModel.accessors[GltfPrimitive.indices];
                    const tinygltf::BufferView& bufferView = GltfModel.bufferViews[accessor.bufferView];
                    const tinygltf::Buffer& buffer = GltfModel.buffers[bufferView.buffer];
                    indexCount += static_cast<uint32_t>(accessor.count);
                    switch (accessor.componentType)
                    {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                    {
                        const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indices.push_back(buf[index]);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                    {
                        const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indices.push_back(buf[index]);
                        }
                        break;
                    }
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                    {
                        const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indices.push_back(buf[index]);
                        }
                        break;
                    }
                    default:
                        std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
                        return;
                    }

                    std::shared_ptr<Texture> defaultTexture = std::make_shared<Texture>(vtDevice, "textures/white.png");
                    std::shared_ptr<Texture> defaultNormalTexture = std::make_shared<Texture>(vtDevice, "textures/normal.png");
                    std::shared_ptr<Texture> defaultMetallicRoughnessTexture = std::make_shared<Texture>(vtDevice, "textures/metallicRoughness.png");

                    PBRMaterial material = {};
                    if (GltfPrimitive.material != -1)
                    {
                        tinygltf::Material& primitiveMaterial = GltfModel.materials[GltfPrimitive.material];
                        if (primitiveMaterial.pbrMetallicRoughness.baseColorTexture.index != -1)
                        {
                            uint32_t textureIndex = primitiveMaterial.pbrMetallicRoughness.baseColorTexture.index;  // Get the texture index
                            uint32_t imageIndex = GltfModel.textures[textureIndex].source;                          // Get the image index
                            material.base_color_texture = images[imageIndex];                                       // Set Base Color Texture
                            material.pbr_parameters.has_base_color_texture = 1;                                     // Set Has_Base_Color Parameter
                        }
                        else
                        {
                            material.base_color_texture = defaultTexture;
                            material.pbr_parameters.has_base_color_texture = 0;
                            auto color = primitiveMaterial.pbrMetallicRoughness.baseColorFactor;
                            material.pbr_parameters.base_color_factor = { color[0], color[1], color[2], color[3] };
                        }

                        if (primitiveMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
                        {
                            uint32_t textureIndex = primitiveMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;  // Get the texture index
                            uint32_t imageIndex = GltfModel.textures[textureIndex].source;                                  // Get the image index
                            material.metallic_roughness_texture = images[imageIndex];                                       // Set Base Color Texture
                            material.pbr_parameters.has_metallic_roughness_texture = 1;                                     // Set Has_Base_Color Parameter
                        }
                        else
                        {
                            material.metallic_roughness_texture = defaultMetallicRoughnessTexture;
                            material.pbr_parameters.has_metallic_roughness_texture = 0;
                            material.pbr_parameters.metallic_factor = primitiveMaterial.pbrMetallicRoughness.metallicFactor;
                            material.pbr_parameters.roughness_factor = primitiveMaterial.pbrMetallicRoughness.roughnessFactor;
                        }

                        if (primitiveMaterial.normalTexture.index != -1)
                        {
                            uint32_t textureIndex = primitiveMaterial.normalTexture.index;
                            uint32_t imageIndex = GltfModel.textures[textureIndex].source;
                            material.normal_texture = images[imageIndex];
                            material.pbr_parameters.has_normal_texture = 1;
                            material.pbr_parameters.scale = primitiveMaterial.normalTexture.scale;
                        }
                        else
                        {
                            material.normal_texture = defaultNormalTexture;
                            material.pbr_parameters.has_normal_texture = 0;
                        }

                        if (primitiveMaterial.occlusionTexture.index != -1)
                        {
                            uint32_t textureIndex = primitiveMaterial.occlusionTexture.index;
                            uint32_t imageIndex = GltfModel.textures[textureIndex].source;
                            material.occlusion_texture = images[imageIndex];
                            material.pbr_parameters.has_occlusion_texture = 1;
                            material.pbr_parameters.strength = primitiveMaterial.occlusionTexture.strength;
                        }
                        else
                        {
                            material.occlusion_texture = defaultTexture;
                            material.pbr_parameters.has_occlusion_texture = 0;
                            material.pbr_parameters.strength = primitiveMaterial.occlusionTexture.strength;
                        }

                        if (primitiveMaterial.emissiveTexture.index != -1)
                        {
                            uint32_t textureIndex = primitiveMaterial.emissiveTexture.index;
                            uint32_t imageIndex = GltfModel.textures[textureIndex].source;
                            material.emissive_texture = images[imageIndex];
                            material.pbr_parameters.has_emissive_texture = 1;
                        }
                        else
                        {
                            material.emissive_texture = defaultTexture;
                            material.pbr_parameters.has_emissive_texture = 0;
                            auto color = primitiveMaterial.emissiveFactor;
                            material.pbr_parameters.emissive_factor = { color[0], color[1], color[2] };
                        }

                        material.pbr_parameters.alpha_cut_off = primitiveMaterial.alphaCutoff;
                        material.pbr_parameters.alpha_mode = 0.f;//static_cast<float>(primitiveMaterial.alphaMode);
                    }
                    else
                    {
                        // TODO: USE SPECIFIC TEXTURE FOR METALLIC AND NORMAL
                        material.base_color_texture = defaultTexture;
                        material.metallic_roughness_texture = defaultMetallicRoughnessTexture;
                        material.normal_texture = defaultNormalTexture;
                        material.occlusion_texture = defaultTexture;
                        material.emissive_texture = defaultTexture;
                    }

                    VtBuffer stagingBuffer{
                        vtDevice,
                        sizeof(PBRParameters),
                        1,
                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
                    };

                    stagingBuffer.map();
                    stagingBuffer.writeToBuffer(&material.pbr_parameters);

                    material.pbr_parameters_buffer = std::make_unique<VtBuffer>(
                        vtDevice,
                        sizeof(PBRParameters),
                        1,
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                        );

                    vtDevice.copyBuffer(stagingBuffer.getBuffer(), material.pbr_parameters_buffer->getBuffer(), sizeof(PBRParameters));

                    VkDescriptorImageInfo baseColorImageInfo = material.base_color_texture->getDescriptorImageInfo();
                    VkDescriptorImageInfo metallicRoughnessImageInfo = material.metallic_roughness_texture->getDescriptorImageInfo();
                    VkDescriptorImageInfo normalImageInfo = material.normal_texture->getDescriptorImageInfo();
                    VkDescriptorImageInfo occlusionImageInfo = material.occlusion_texture->getDescriptorImageInfo();
                    VkDescriptorImageInfo emissiveImageInfo = material.emissive_texture->getDescriptorImageInfo();
                    VkDescriptorBufferInfo pbrParametersBufferInfo = material.pbr_parameters_buffer->getDescriptorInfo();

                    VtDescriptorWriter(materialSetLayout, descriptorPool)
                        .writeImage(0, &baseColorImageInfo)
                        .writeImage(1, &metallicRoughnessImageInfo)
                        .writeImage(2, &normalImageInfo)
                        .writeImage(3, &occlusionImageInfo)
                        .writeImage(4, &emissiveImageInfo)
                        .writeBuffer(5, &pbrParametersBufferInfo)
                        .build(material.descriptor_set);

                    Primitive primitive{};
                    primitive.firstVertex = vertexOffset;
                    primitive.vertexCount = vertexCount;
                    primitive.indexCount = indexCount;
                    primitive.firstIndex = indexOffset;
                    primitive.material = material;
                    primitives.push_back(primitive);
                    vertexOffset += vertexCount;
                    indexOffset += indexCount;
                }
            }
            createVertexBuffers(vertices);
            createIndexBuffers(indices);
        }
    }
}
