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

#include <fx/gltf.h>

#ifndef ENGINE_DIR
#define ENGINE_DIR ""
#endif

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
        fx::gltf::Document doc = fx::gltf::LoadFromText(filepath);
        std::filesystem::path path = std::filesystem::path(filepath);

        for (auto& image : doc.images)
        {
            images.push_back(std::make_shared<Texture>(vtDevice, path.parent_path().append(image.uri).generic_string()));
        }

        uint32_t vertexOffset = 0;
        uint32_t indexOffset = 0;

        for (auto& mesh : doc.meshes)
        {
            for (auto& primitive : mesh.primitives)
            {
                uint32_t vertexCount = 0;
                uint32_t indexCount = 0;

                const float* positionBuffer = nullptr;
                const float* normalsBuffer = nullptr;
                const float* texCoordsBuffer = nullptr;
                const float* tangentsBuffer = nullptr;

                for (auto const& attribute : primitive.attributes)
                {
                    if (attribute.first == "POSITION")
                    {
                        const fx::gltf::Accessor& accessor = doc.accessors[primitive.attributes.find(
                            "POSITION")->second];
                        const fx::gltf::BufferView& view = doc.bufferViews[accessor.bufferView];
                        positionBuffer = reinterpret_cast<const float*>(&(doc.buffers[view.buffer].data[
                            accessor.byteOffset + view.byteOffset]));
                        vertexCount = accessor.count;
                    }

                    if (attribute.first == "NORMAL")
                    {
                        const fx::gltf::Accessor& accessor = doc.accessors[primitive.attributes.find("NORMAL")->second];
                        const fx::gltf::BufferView& view = doc.bufferViews[accessor.bufferView];
                        normalsBuffer = reinterpret_cast<const float*>(&(doc.buffers[view.buffer].data[
                            accessor.byteOffset + view.byteOffset]));
                    }

                    if (attribute.first == "TEXCOORD_0")
                    {
                        const fx::gltf::Accessor& accessor = doc.accessors[primitive.attributes.find(
                            "TEXCOORD_0")->second];
                        const fx::gltf::BufferView& view = doc.bufferViews[accessor.bufferView];
                        texCoordsBuffer = reinterpret_cast<const float*>(&(doc.buffers[view.buffer].data[
                            accessor.byteOffset + view.byteOffset]));
                    }

                    if (attribute.first == "TANGENT")
                    {
                        const fx::gltf::Accessor& accessor = doc.accessors[primitive.attributes.find(
                            "TANGENT")->second];
                        const fx::gltf::BufferView& view = doc.bufferViews[accessor.bufferView];
                        tangentsBuffer = reinterpret_cast<const float*>(&(doc.buffers[view.buffer].data[
                            accessor.byteOffset + view.byteOffset]));
                    }
                }

                std::shared_ptr<Texture> defaultTexture = std::make_shared<Texture>(vtDevice, "textures/white.png");

                PBRMaterial material = {};
                if (primitive.material != -1)
                {
                    fx::gltf::Material& primitiveMaterial = doc.materials[primitive.material];
                    if (!primitiveMaterial.pbrMetallicRoughness.baseColorTexture.empty())
                    {
                        uint32_t textureIndex = primitiveMaterial.pbrMetallicRoughness.baseColorTexture.index;
                        uint32_t imageIndex = doc.textures[textureIndex].source;
                        material.base_color_texture = images[imageIndex];
                        material.pbr_parameters.has_base_color_texture = 1;
                    }
                    else
                    {
                        material.base_color_texture = defaultTexture;
                        material.pbr_parameters.has_base_color_texture = 0;
                        auto color = primitiveMaterial.pbrMetallicRoughness.baseColorFactor;
                        material.pbr_parameters.base_color_factor = { color[0], color[1], color[2], color[3] };
                    }

                    if (!primitiveMaterial.pbrMetallicRoughness.metallicRoughnessTexture.empty())
                    {
                        uint32_t textureIndex = primitiveMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
                        uint32_t imageIndex = doc.textures[textureIndex].source;
                        material.metallic_roughness_texture = images[imageIndex];
                        material.pbr_parameters.has_metallic_roughness_texture = 1;
                    }
                    else
                    {
                        material.metallic_roughness_texture = defaultTexture;
                        material.pbr_parameters.has_metallic_roughness_texture = 0;
                        material.pbr_parameters.metallic_factor = primitiveMaterial.pbrMetallicRoughness.metallicFactor;
                        material.pbr_parameters.roughness_factor = primitiveMaterial.pbrMetallicRoughness.roughnessFactor;
                    }

                    if (!primitiveMaterial.normalTexture.empty())
                    {
                        uint32_t textureIndex = primitiveMaterial.normalTexture.index;
                        uint32_t imageIndex = doc.textures[textureIndex].source;
                        material.normal_texture = images[imageIndex];
                        material.pbr_parameters.has_normal_texture = 1;
                        material.pbr_parameters.scale = primitiveMaterial.normalTexture.scale;
                    }
                    else
                    {
                        material.normal_texture = defaultTexture;
                        material.pbr_parameters.has_normal_texture = 0;
                    }

                    if (!primitiveMaterial.occlusionTexture.empty())
                    {
                        uint32_t textureIndex = primitiveMaterial.occlusionTexture.index;
                        uint32_t imageIndex = doc.textures[textureIndex].source;
                        material.occlusion_texture = images[imageIndex];
                        material.pbr_parameters.has_occlusion_texture = 1;
                    }
                    else
                    {
                        material.occlusion_texture = defaultTexture;
                        material.pbr_parameters.has_occlusion_texture = 0;
                        material.pbr_parameters.strength = primitiveMaterial.occlusionTexture.strength;
                    }

                    if (!primitiveMaterial.emissiveTexture.empty())
                    {
                        uint32_t textureIndex = primitiveMaterial.emissiveTexture.index;
                        uint32_t imageIndex = doc.textures[textureIndex].source;
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
                    material.pbr_parameters.alpha_mode = static_cast<float>(primitiveMaterial.alphaMode);
                }
                else
                {
                    material.base_color_texture = defaultTexture;
                    material.metallic_roughness_texture = defaultTexture;
                    material.normal_texture = defaultTexture;
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

                for (size_t v = 0; v < vertexCount; v++)
                {
                    Vertex vertex{};
                    vertex.position = glm::make_vec3(&positionBuffer[v * 3]);
                    vertex.normal = glm::normalize(
                        glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
                    vertex.tangent = glm::vec4(
                        tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f));;
                    vertex.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec2(0.0f);
                    vertices.push_back(vertex);
                }

                {
                    const fx::gltf::Accessor& accessor = doc.accessors[primitive.indices];
                    const fx::gltf::BufferView& bufferView = doc.bufferViews[accessor.bufferView];
                    const fx::gltf::Buffer& buffer = doc.buffers[bufferView.buffer];

                    indexCount += static_cast<uint32_t>(accessor.count);

                    switch (accessor.componentType)
                    {
                    case fx::gltf::Accessor::ComponentType::UnsignedInt:
                    {
                        const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset +
                            bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indices.push_back(buf[index]);
                        }
                        break;
                    }
                    case fx::gltf::Accessor::ComponentType::UnsignedShort:
                    {
                        const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset +
                            bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indices.push_back(buf[index]);
                        }
                        break;
                    }
                    case fx::gltf::Accessor::ComponentType::UnsignedByte:
                    {
                        const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset +
                            bufferView.byteOffset]);
                        for (size_t index = 0; index < accessor.count; index++)
                        {
                            indices.push_back(buf[index]);
                        }
                        break;
                    }
                    default:
                        return;
                    }
                }

                Primitive mesh_primitive{};
                mesh_primitive.firstVertex = vertexOffset;
                mesh_primitive.vertexCount = vertexCount;
                mesh_primitive.indexCount = indexCount;
                mesh_primitive.firstIndex = indexOffset;
                mesh_primitive.material = std::move(material);
                primitives.push_back(mesh_primitive);

                vertexOffset += vertexCount;
                indexOffset += indexCount;

            }
        }

        createVertexBuffers(vertices);
        createIndexBuffers(indices);
    }
}
