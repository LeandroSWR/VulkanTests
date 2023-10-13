#include "RTSetup.hpp"

#include "nvvk/buffers_vk.hpp"


namespace vt {

    RTSetup::RTSetup(VtModel& model, VtDevice& device, VtGameObject::Map& gameObjects, VtSwapChain& swapChain) : m_model(model), m_device(device), gameObjects(gameObjects), swapChain(swapChain) 
    {
        d_malloc.init(device.getVulkanInstance(), device.device(), device.getPhysicalDevice());
    }
    
    RTSetup::~RTSetup() {}

    ////--------------------------------------------------------------------------------------------------
    //// Initialize Vulkan ray tracing
    //// #VKRay
    //void RTSetup::initRayTracing()
    //{
    //    m_rtBuilder.setup(m_device.device(), &m_alloc, VK_QUEUE_FAMILY_IGNORED);
    //}

    ////--------------------------------------------------------------------------------------------------
    //// Convert an OBJ model into the ray tracing geometry used to build the BLAS
    ////
    //auto RTSetup::objectToVkGeometryKHR(VtModel& model)
    //{
    //    // BLAS builder requires raw device addresses.
    //    VkDeviceAddress vertexAddress = nvvk::getBufferDeviceAddress(m_device.device(), model.getVertexBuffer());
    //    VkDeviceAddress indexAddress = nvvk::getBufferDeviceAddress(m_device.device(), model.getIndexBuffer());

    //    uint32_t maxPrimitiveCount = model.getnIndices() / 3;

    //    // Describe buffer as array of VertexObj.
    //    VkAccelerationStructureGeometryTrianglesDataKHR triangles{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR };
    //    triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;  // vec3 vertex position data.
    //    triangles.vertexData.deviceAddress = vertexAddress;
    //    triangles.vertexStride = sizeof(VtModel::Vertex);
    //    // Describe index data (32-bit unsigned int)
    //    triangles.indexType = VK_INDEX_TYPE_UINT32;
    //    triangles.indexData.deviceAddress = indexAddress;
    //    // Indicate identity transform by setting transformData to null device pointer.
    //    //triangles.transformData = {};
    //    triangles.maxVertex = model.getnVertices() - 1;

    //    // Identify the above data as containing opaque triangles.
    //    VkAccelerationStructureGeometryKHR asGeom{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
    //    asGeom.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    //    asGeom.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
    //    asGeom.geometry.triangles = triangles;

    //    // The entire array will be used to build the BLAS.
    //    VkAccelerationStructureBuildRangeInfoKHR offset;
    //    offset.firstVertex = 0;
    //    offset.primitiveCount = maxPrimitiveCount;
    //    offset.primitiveOffset = 0;
    //    offset.transformOffset = 0;

    //    // Our blas is made from only one geometry, but could be made of many geometries
    //    nvvk::RaytracingBuilderKHR::BlasInput input;
    //    input.asGeometry.emplace_back(asGeom);
    //    input.asBuildOffsetInfo.emplace_back(offset);

    //    return input;
    //}

    ////--------------------------------------------------------------------------------------------------
    //// Create the BLAS
    ////
    //void RTSetup::createBottomLevelAS()
    //{
    //    // BLAS - Storing each primitive in a geometry
    //    std::vector<nvvk::RaytracingBuilderKHR::BlasInput> allBlas;
    //    allBlas.reserve(1);
    //    for (const auto& obj : gameObjects)
    //    {
    //        auto blas = objectToVkGeometryKHR(*obj.second.model);

    //        // We could add more geometry in each BLAS, but we add only one for now
    //        allBlas.emplace_back(blas);
    //    }

    //    //auto blas = objectToVkGeometryKHR(m_model);

    //    // We could add more geometry in each BLAS, but we add only one for now
    //    //allBlas.emplace_back(blas);

    //    m_rtBuilder.buildBlas(allBlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
    //}

    //nvmath::mat4f RTSetup::convertGLMToNVMATH(const glm::mat4& glmMatrix)
    //{
    //    nvmath::mat4f nvMatrix;

    //    for (int i = 0; i < 4; ++i)
    //    {
    //        for (int j = 0; j < 4; ++j)
    //        {
    //            nvMatrix.mat_array[j + (4 * i)] = glmMatrix[i][j];
    //        }
    //    }

    //    return nvMatrix;
    //}

    ////--------------------------------------------------------------------------------------------------
    //// Create the TLAS
    ////
    //void RTSetup::createTopLevelAS()
    //{
    //    std::vector<VkAccelerationStructureInstanceKHR> tlas;
    //    tlas.reserve(gameObjects.size());
    //    for (auto& inst : gameObjects)
    //    {
    //        VkAccelerationStructureInstanceKHR rayInst{};
    //        rayInst.transform = nvvk::toTransformMatrixKHR(convertGLMToNVMATH(inst.second.transform.mat4()));  // Position of the instance
    //        rayInst.instanceCustomIndex = inst.first;                               // gl_InstanceCustomIndexEXT
    //        rayInst.accelerationStructureReference = m_rtBuilder.getBlasDeviceAddress(inst.first);
    //        rayInst.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    //        rayInst.mask = 0xFF;       //  Only be hit if rayMask & instance.mask != 0
    //        rayInst.instanceShaderBindingTableRecordOffset = 0;  // We will use the same hit group for all objects
    //        tlas.emplace_back(rayInst);
    //    }
    //    m_rtBuilder.buildTlas(tlas, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
    //}

    ////--------------------------------------------------------------------------------------------------
    //// This descriptor set holds the Acceleration structure and the output image
    ////
    //void RTSetup::createRtDescriptorSet()
    //{
    //    m_rtDescSetLayoutBind.addBinding(RtxBindings::eTlas, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,
    //        VK_SHADER_STAGE_RAYGEN_BIT_KHR);  // TLAS
    //    m_rtDescSetLayoutBind.addBinding(RtxBindings::eOutImage, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
    //        VK_SHADER_STAGE_RAYGEN_BIT_KHR);  // Output image

    //    m_rtDescPool = m_rtDescSetLayoutBind.createPool(m_device.device());
    //    m_rtDescSetLayout = m_rtDescSetLayoutBind.createLayout(m_device.device());

    //    VkDescriptorSetAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
    //    allocateInfo.descriptorPool = m_rtDescPool;
    //    allocateInfo.descriptorSetCount = 1;
    //    allocateInfo.pSetLayouts = &m_rtDescSetLayout;
    //    vkAllocateDescriptorSets(m_device.device(), &allocateInfo, &m_rtDescSet);


    //    VkAccelerationStructureKHR                   tlas = m_rtBuilder.getAccelerationStructure();
    //    VkWriteDescriptorSetAccelerationStructureKHR descASInfo{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR };
    //    descASInfo.accelerationStructureCount = 1;
    //    descASInfo.pAccelerationStructures = &tlas;
    //    VkDescriptorImageInfo imageInfo{ {}, swapChain.getImageView(0), VK_IMAGE_LAYOUT_GENERAL };

    //    std::vector<VkWriteDescriptorSet> writes;
    //    writes.emplace_back(m_rtDescSetLayoutBind.makeWrite(m_rtDescSet, RtxBindings::eTlas, &descASInfo));
    //    writes.emplace_back(m_rtDescSetLayoutBind.makeWrite(m_rtDescSet, RtxBindings::eOutImage, &imageInfo));
    //    vkUpdateDescriptorSets(m_device.device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    //}

    ////--------------------------------------------------------------------------------------------------
    //// Writes the output image to the descriptor set
    //// - Required when changing resolution
    ////
    //void RTSetup::updateRtDescriptorSet()
    //{
    //    // (1) Output buffer
    //    VkDescriptorImageInfo imageInfo{ {}, swapChain.getImageView(0), VK_IMAGE_LAYOUT_GENERAL };
    //    VkWriteDescriptorSet  wds = m_rtDescSetLayoutBind.makeWrite(m_rtDescSet, RtxBindings::eOutImage, &imageInfo);
    //    vkUpdateDescriptorSets(m_device.device(), 1, &wds, 0, nullptr);
    //}
}