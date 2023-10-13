#include <sstream>

#include "RTSetup.hpp"

#include "nvh/alignment.hpp"
#include "nvh/fileoperations.hpp"
#include "nvvk/shaders_vk.hpp"
#include "nvvk/buffers_vk.hpp"

extern std::vector<std::string> defaultSearchPaths;

namespace vt {

    RTSetup::RTSetup(VtDevice& device, VtGameObject::Map& gameObjects, VtSwapChain& swapChain) : m_device(device), gameObjects(gameObjects), swapChain(swapChain)
    {
        m_alloc.init(device.getVulkanInstance(), device.device(), device.getPhysicalDevice());

        // Camera matrices
        m_descSetLayoutBind.addBinding(SceneBindings::eGlobals, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR);
        // Obj descriptions
        m_descSetLayoutBind.addBinding(SceneBindings::eObjDescs, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);
        // Textures
        m_descSetLayoutBind.addBinding(SceneBindings::eTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 71,
            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR);

        m_descSetLayout = m_descSetLayoutBind.createLayout(m_device.device());
        m_descSet = nvvk::allocateDescriptorSet(m_device.device(), m_descSetLayoutBind.createPool(m_device.device(), 1), m_descSetLayout);
    }
    
    RTSetup::~RTSetup() {}

    //--------------------------------------------------------------------------------------------------
    // Initialize Vulkan ray tracing
    // #VKRay
    void RTSetup::initRayTracing()
    {
        m_rtBuilder.setup(m_device.device(), &m_alloc, VK_QUEUE_FAMILY_IGNORED);
    }

    //--------------------------------------------------------------------------------------------------
    // Convert an OBJ model into the ray tracing geometry used to build the BLAS
    //
    auto RTSetup::objectToVkGeometryKHR(VtModel& model)
    {
        // BLAS builder requires raw device addresses.
        VkDeviceAddress vertexAddress = nvvk::getBufferDeviceAddress(m_device.device(), model.getVertexBuffer());
        VkDeviceAddress indexAddress = nvvk::getBufferDeviceAddress(m_device.device(), model.getIndexBuffer());

        uint32_t maxPrimitiveCount = model.getnIndices() / 3;

        // Describe buffer as array of VertexObj.
        VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
        triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 vertex position data.
        triangles.vertexData.deviceAddress = vertexAddress;
        triangles.vertexStride = sizeof(VtModel::Vertex);
        // Describe index data (32-bit unsigned int)
        triangles.indexType = VK_INDEX_TYPE_UINT32;
        triangles.indexData.deviceAddress = indexAddress;
        // Indicate identity transform by setting transformData to null device pointer.
        //triangles.transformData = {};
        triangles.maxVertex = model.getnVertices() - 1;

        // Identify the above data as containing opaque triangles.
        VkAccelerationStructureGeometryKHR asGeom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        asGeom.geometry.triangles = triangles;

        // The entire array will be used to build the BLAS.
        VkAccelerationStructureBuildRangeInfoKHR offset;
        offset.firstVertex = 0;
        offset.primitiveCount = maxPrimitiveCount;
        offset.primitiveOffset = 0;
        offset.transformOffset = 0;

        // Our blas is made from only one geometry, but could be made of many geometries
        nvvk::RaytracingBuilderKHR::BlasInput input;
        input.asGeometry.emplace_back(asGeom);
        input.asBuildOffsetInfo.emplace_back(offset);

        return input;
    }

    //--------------------------------------------------------------------------------------------------
    // Create the BLAS
    //
    void RTSetup::createBottomLevelAS()
    {
        // BLAS - Storing each primitive in a geometry
        std::vector<nvvk::RaytracingBuilderKHR::BlasInput> allBlas;
        allBlas.reserve(gameObjects.size());
        for (const auto& obj : gameObjects)
        {
            auto& model = obj.second;
            if (model.model == nullptr) continue;
            auto blas = objectToVkGeometryKHR(*model.model);

            // We could add more geometry in each BLAS, but we add only one for now
            allBlas.emplace_back(blas);
        }

        m_rtBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
    }

    nvmath::mat4f RTSetup::convertGLMToNVMATH(const glm::mat4& glmMatrix)
    {
        nvmath::mat4f nvMatrix;

        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                nvMatrix.mat_array[j + (4 * i)] = glmMatrix[i][j];
            }
        }

        return nvMatrix;
    }

    //--------------------------------------------------------------------------------------------------
    // Create the TLAS
    //
    void RTSetup::createTopLevelAS()
    {
        std::vector<VkAccelerationStructureInstanceKHR> tlas;
        tlas.reserve(gameObjects.size());
        for (auto& inst : gameObjects)
        {
            VkAccelerationStructureInstanceKHR rayInst{};
            rayInst.transform = nvvk::toTransformMatrixKHR(convertGLMToNVMATH(inst.second.transform.mat4()));  // Position of the instance
            rayInst.instanceCustomIndex = inst.first;                               // gl_InstanceCustomIndexEXT
            rayInst.accelerationStructureReference = m_rtBuilder.getBlasDeviceAddress(inst.first);
            rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
            rayInst.mask = 0xFF;       //  Only be hit if rayMask & instance.mask != 0
            rayInst.instanceShaderBindingTableRecordOffset = 0;  // We will use the same hit group for all objects
            tlas.emplace_back(rayInst);
        }
        m_rtBuilder.buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
    }

    //--------------------------------------------------------------------------------------------------
    // This descriptor set holds the Acceleration structure and the output image
    //
    void RTSetup::createRtDescriptorSet()
    {
        m_rtDescSetLayoutBind.addBinding(RtxBindings::eTlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR);  // TLAS
        m_rtDescSetLayoutBind.addBinding(RtxBindings::eOutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR);  // Output image

        m_rtDescPool = m_rtDescSetLayoutBind.createPool(m_device.device());
        m_rtDescSetLayout = m_rtDescSetLayoutBind.createLayout(m_device.device());

        VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocateInfo.descriptorPool = m_rtDescPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &m_rtDescSetLayout;
        vkAllocateDescriptorSets(m_device.device(), &allocateInfo, &m_rtDescSet);


        VkAccelerationStructureKHR                   tlas = m_rtBuilder.getAccelerationStructure();
        VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
        descASInfo.accelerationStructureCount = 1;
        descASInfo.pAccelerationStructures = &tlas;
        VkDescriptorImageInfo imageInfo{ {}, swapChain.getImageView(0), VK_IMAGE_LAYOUT_GENERAL };

        std::vector<VkWriteDescriptorSet> writes;
        writes.emplace_back(m_rtDescSetLayoutBind.makeWrite(m_rtDescSet, RtxBindings::eTlas, &descASInfo));
        writes.emplace_back(m_rtDescSetLayoutBind.makeWrite(m_rtDescSet, RtxBindings::eOutImage, &imageInfo));
        vkUpdateDescriptorSets(m_device.device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    //--------------------------------------------------------------------------------------------------
    // Writes the output image to the descriptor set
    // - Required when changing resolution
    //
    void RTSetup::updateRtDescriptorSet()
    {
        // (1) Output buffer
        VkDescriptorImageInfo imageInfo{ {}, swapChain.getImageView(0), VK_IMAGE_LAYOUT_GENERAL };
        VkWriteDescriptorSet  wds = m_rtDescSetLayoutBind.makeWrite(m_rtDescSet, RtxBindings::eOutImage, &imageInfo);
        vkUpdateDescriptorSets(m_device.device(), 1, &wds, 0, nullptr);
    }

    //--------------------------------------------------------------------------------------------------
// Pipeline for the ray tracer: all shaders, raygen, chit, miss
//
    void RTSetup::createRtPipeline()
    {
        enum StageIndices
        {
            eRaygen,
            eMiss,
            eMiss2,
            eClosestHit,
            eShaderGroupCount
        };

        // All stages
        std::array<VkPipelineShaderStageCreateInfo, eShaderGroupCount> stages{};
        VkPipelineShaderStageCreateInfo stage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
        stage.pName = "main";  // All the same entry point
        // Raygen
        stage.module = nvvk::createShaderModule(m_device.device(), nvh::loadFile("shaders/raytrace.rgen.spv", true, defaultSearchPaths, true));
        stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
        stages[eRaygen] = stage;
        // Miss
        stage.module = nvvk::createShaderModule(m_device.device(), nvh::loadFile("shaders/raytrace.rmiss.spv", true, defaultSearchPaths, true));
        stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        stages[eMiss] = stage;
        // The second miss shader is invoked when a shadow ray misses the geometry. It simply indicates that no occlusion has been found
        stage.module =
            nvvk::createShaderModule(m_device.device(), nvh::loadFile("shaders/raytraceShadow.rmiss.spv", true, defaultSearchPaths, true));
        stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
        stages[eMiss2] = stage;
        // Hit Group - Closest Hit
        stage.module = nvvk::createShaderModule(m_device.device(), nvh::loadFile("shaders/raytrace.rchit.spv", true, defaultSearchPaths, true));
        stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
        stages[eClosestHit] = stage;


        // Shader groups
        VkRayTracingShaderGroupCreateInfoKHR group{ VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR };
        group.anyHitShader = VK_SHADER_UNUSED_KHR;
        group.closestHitShader = VK_SHADER_UNUSED_KHR;
        group.generalShader = VK_SHADER_UNUSED_KHR;
        group.intersectionShader = VK_SHADER_UNUSED_KHR;

        // Raygen
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group.generalShader = eRaygen;
        m_rtShaderGroups.push_back(group);

        // Miss
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group.generalShader = eMiss;
        m_rtShaderGroups.push_back(group);

        // Shadow Miss
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        group.generalShader = eMiss2;
        m_rtShaderGroups.push_back(group);

        // closest hit shader
        group.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
        group.generalShader = VK_SHADER_UNUSED_KHR;
        group.closestHitShader = eClosestHit;
        m_rtShaderGroups.push_back(group);

        // Push constant: we want to be able to update constants used by the shaders
        VkPushConstantRange pushConstant{ VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
                                         0, sizeof(PushConstantRay) };


        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstant;

        // Descriptor sets: one specific to ray tracing, and one shared with the rasterization pipeline
        std::vector<VkDescriptorSetLayout> rtDescSetLayouts = { m_rtDescSetLayout, m_descSetLayout };
        pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(rtDescSetLayouts.size());
        pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

        vkCreatePipelineLayout(m_device.device(), &pipelineLayoutCreateInfo, nullptr, &m_rtPipelineLayout);

        // Assemble the shader stages and recursion depth info into the ray tracing pipeline
        VkRayTracingPipelineCreateInfoKHR rayPipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
        rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size());  // Stages are shaders
        rayPipelineInfo.pStages = stages.data();

        // In this case, m_rtShaderGroups.size() == 4: we have one raygen group,
        // two miss shader groups, and one hit group.
        rayPipelineInfo.groupCount = static_cast<uint32_t>(m_rtShaderGroups.size());
        rayPipelineInfo.pGroups = m_rtShaderGroups.data();

        // The ray tracing process can shoot rays from the camera, and a shadow ray can be shot from the
        // hit points of the camera rays, hence a recursion level of 2. This number should be kept as low
        // as possible for performance reasons. Even recursive ray tracing should be flattened into a loop
        // in the ray generation to avoid deep recursion.
        rayPipelineInfo.maxPipelineRayRecursionDepth = 2;  // Ray depth
        rayPipelineInfo.layout = m_rtPipelineLayout;

        vkCreateRayTracingPipelinesKHR(m_device.device(), {}, {}, 1, & rayPipelineInfo, nullptr, & m_rtPipeline);


        // Spec only guarantees 1 level of "recursion". Check for that sad possibility here.
        if (m_rtProperties.maxRayRecursionDepth <= 1)
        {
            throw std::runtime_error("Device fails to support ray recursion (m_rtProperties.maxRayRecursionDepth <= 1)");
        }

        for (auto& s : stages)
            vkDestroyShaderModule(m_device.device(), s.module, nullptr);
    }

    //--------------------------------------------------------------------------------------------------
// The Shader Binding Table (SBT)
// - getting all shader handles and write them in a SBT buffer
// - Besides exception, this could be always done like this
//
    void RTSetup::createRtShaderBindingTable()
    {
        uint32_t missCount{ 2 };
        uint32_t hitCount{ 1 };
        auto     handleCount = 1 + missCount + hitCount;
        uint32_t handleSize = m_rtProperties.shaderGroupHandleSize;

        // The SBT (buffer) need to have starting groups to be aligned and handles in the group to be aligned.
        uint32_t handleSizeAligned = nvh::align_up(handleSize, m_rtProperties.shaderGroupHandleAlignment);

        m_rgenRegion.stride = nvh::align_up(handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);
        m_rgenRegion.size = m_rgenRegion.stride;  // The size member of pRayGenShaderBindingTable must be equal to its stride member
        m_missRegion.stride = handleSizeAligned;
        m_missRegion.size = nvh::align_up(missCount * handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);
        m_hitRegion.stride = handleSizeAligned;
        m_hitRegion.size = nvh::align_up(hitCount * handleSizeAligned, m_rtProperties.shaderGroupBaseAlignment);

        // Get the shader group handles
        uint32_t             dataSize = handleCount * handleSize;
        std::vector<uint8_t> handles(dataSize);
        auto result = vkGetRayTracingShaderGroupHandlesKHR(m_device.device(), m_rtPipeline, 0, handleCount, dataSize, handles.data());
        assert(result == VK_SUCCESS);

        // Allocate a buffer for storing the SBT.
        VkDeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size + m_hitRegion.size + m_callRegion.size;
        m_rtSBTBuffer = m_alloc.createBuffer(sbtSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            | VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        m_debug.setObjectName(m_rtSBTBuffer.buffer, std::string("SBT"));  // Give it a debug name for NSight.

        // Find the SBT addresses of each group
        VkBufferDeviceAddressInfo info{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, m_rtSBTBuffer.buffer };
        VkDeviceAddress           sbtAddress = vkGetBufferDeviceAddress(m_device.device(), &info);
        m_rgenRegion.deviceAddress = sbtAddress;
        m_missRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
        m_hitRegion.deviceAddress = sbtAddress + m_rgenRegion.size + m_missRegion.size;

        // Helper to retrieve the handle data
        auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

        // Map the SBT buffer and write in the handles.
        auto* pSBTBuffer = reinterpret_cast<uint8_t*>(m_alloc.map(m_rtSBTBuffer));
        uint8_t* pData{ nullptr };
        uint32_t handleIdx{ 0 };
        // Raygen
        pData = pSBTBuffer;
        memcpy(pData, getHandle(handleIdx++), handleSize);
        // Miss
        pData = pSBTBuffer + m_rgenRegion.size;
        for (uint32_t c = 0; c < missCount; c++)
        {
            memcpy(pData, getHandle(handleIdx++), handleSize);
            pData += m_missRegion.stride;
        }
        // Hit
        pData = pSBTBuffer + m_rgenRegion.size + m_missRegion.size;
        for (uint32_t c = 0; c < hitCount; c++)
        {
            memcpy(pData, getHandle(handleIdx++), handleSize);
            pData += m_hitRegion.stride;
        }

        m_alloc.unmap(m_rtSBTBuffer);
        m_alloc.finalizeAndReleaseStaging();
    }

    //--------------------------------------------------------------------------------------------------
// Ray Tracing the scene
//
    void RTSetup::raytrace(const VkCommandBuffer& cmdBuf, const nvmath::vec4f& clearColor)
    {
        m_debug.beginLabel(cmdBuf, "Ray trace");
        // Initializing push constant values
        m_pcRay.clearColor = clearColor;
        m_pcRay.lightPosition = m_pcRaster.lightPosition;
        m_pcRay.lightIntensity = m_pcRaster.lightIntensity;
        m_pcRay.lightType = m_pcRaster.lightType;

        std::vector<VkDescriptorSet> descSets{ m_rtDescSet, };
        vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);
        vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipelineLayout, 0,
            (uint32_t)descSets.size(), descSets.data(), 0, nullptr);
        vkCmdPushConstants(cmdBuf, m_rtPipelineLayout,
            VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
            0, sizeof(PushConstantRay), &m_pcRay);


        vkCmdTraceRaysKHR(cmdBuf, &m_rgenRegion, &m_missRegion, &m_hitRegion, &m_callRegion, swapChain.width(), swapChain.height(), 1);


        m_debug.endLabel(cmdBuf);
    }
}