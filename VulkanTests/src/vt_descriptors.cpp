#include "vt_descriptors.hpp"

// std
#include <cassert>
#include <stdexcept>

namespace vt
{

    // *************** Descriptor Set Layout Builder *********************

    VtDescriptorSetLayout::Builder& VtDescriptorSetLayout::Builder::addBinding(
        uint32_t binding,
        VkDescriptorType descriptorType,
        VkShaderStageFlags stageFlags,
        uint32_t count)
    {
        assert(bindings.count(binding) == 0 && "Binding already in use");
        VkDescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = binding;
        layoutBinding.descriptorType = descriptorType;
        layoutBinding.descriptorCount = count;
        layoutBinding.stageFlags = stageFlags;
        bindings[binding] = layoutBinding;
        return *this;
    }

    std::unique_ptr<VtDescriptorSetLayout> VtDescriptorSetLayout::Builder::build() const
    {
        return std::make_unique<VtDescriptorSetLayout>(vtDevice, bindings);
    }

    // *************** Descriptor Set Layout *********************

    VtDescriptorSetLayout::VtDescriptorSetLayout(
        VtDevice& vtDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
        : vtDevice{ vtDevice }, bindings{ bindings }
    {
        std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
        for (auto kv : bindings)
        {
            setLayoutBindings.push_back(kv.second);
        }

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
        descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

        if (vkCreateDescriptorSetLayout(
            vtDevice.device(),
            &descriptorSetLayoutInfo,
            nullptr,
            &descriptorSetLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }

    VtDescriptorSetLayout::~VtDescriptorSetLayout()
    {
        vkDestroyDescriptorSetLayout(vtDevice.device(), descriptorSetLayout, nullptr);
    }

    // *************** Descriptor Pool Builder *********************

    VtDescriptorPool::Builder& VtDescriptorPool::Builder::addPoolSize(
        VkDescriptorType descriptorType, uint32_t count)
    {
        poolSizes.push_back({ descriptorType, count });
        return *this;
    }

    VtDescriptorPool::Builder& VtDescriptorPool::Builder::setPoolFlags(
        VkDescriptorPoolCreateFlags flags)
    {
        poolFlags = flags;
        return *this;
    }
    VtDescriptorPool::Builder& VtDescriptorPool::Builder::setMaxSets(uint32_t count)
    {
        maxSets = count;
        return *this;
    }

    std::unique_ptr<VtDescriptorPool> VtDescriptorPool::Builder::build() const
    {
        return std::make_unique<VtDescriptorPool>(vtDevice, maxSets, poolFlags, poolSizes);
    }

    // *************** Descriptor Pool *********************

    VtDescriptorPool::VtDescriptorPool(
        VtDevice& vtDevice,
        uint32_t maxSets,
        VkDescriptorPoolCreateFlags poolFlags,
        const std::vector<VkDescriptorPoolSize>& poolSizes)
        : vtDevice{ vtDevice }
    {
        VkDescriptorPoolCreateInfo descriptorPoolInfo{};
        descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        descriptorPoolInfo.pPoolSizes = poolSizes.data();
        descriptorPoolInfo.maxSets = maxSets;
        descriptorPoolInfo.flags = poolFlags;

        if (vkCreateDescriptorPool(vtDevice.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    VtDescriptorPool::~VtDescriptorPool()
    {
        vkDestroyDescriptorPool(vtDevice.device(), descriptorPool, nullptr);
    }

    bool VtDescriptorPool::allocateDescriptorSet(
        const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.pSetLayouts = &descriptorSetLayout;
        allocInfo.descriptorSetCount = 1;

        // Might need to create a "DescriptorPoolManager" class that handles this case, and builds
        // a new pool whenever an old pool fills up.
        // https://vkguide.dev/docs/extra-chapter/abstracting_descriptors/
        if (vkAllocateDescriptorSets(vtDevice.device(), &allocInfo, &descriptor) != VK_SUCCESS)
        {
            return false;
        }
        return true;
    }

    void VtDescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const
    {
        vkFreeDescriptorSets(
            vtDevice.device(),
            descriptorPool,
            static_cast<uint32_t>(descriptors.size()),
            descriptors.data());
    }

    void VtDescriptorPool::resetPool()
    {
        vkResetDescriptorPool(vtDevice.device(), descriptorPool, 0);
    }

    // *************** Descriptor Writer *********************

    VtDescriptorWriter::VtDescriptorWriter(VtDescriptorSetLayout& setLayout, VtDescriptorPool& pool)
        : setLayout{ setLayout }, pool{ pool }
    {
    }

    VtDescriptorWriter& VtDescriptorWriter::writeBuffer(
        uint32_t binding, VkDescriptorBufferInfo* bufferInfo)
    {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];

        assert(bindingDescription.descriptorCount == 1 && 
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pBufferInfo = bufferInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    VtDescriptorWriter& VtDescriptorWriter::writeImage(
        uint32_t binding, VkDescriptorImageInfo* imageInfo)
    {
        assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

        auto& bindingDescription = setLayout.bindings[binding];

        assert(
            bindingDescription.descriptorCount == 1 &&
            "Binding single descriptor info, but binding expects multiple");

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.descriptorType = bindingDescription.descriptorType;
        write.dstBinding = binding;
        write.pImageInfo = imageInfo;
        write.descriptorCount = 1;

        writes.push_back(write);
        return *this;
    }

    bool VtDescriptorWriter::build(VkDescriptorSet& set)
    {
        bool success = pool.allocateDescriptorSet(setLayout.getDescriptorSetLayout(), set);
        if (!success)
        {
            return false;
        }
        overwrite(set);
        return true;
    }

    void VtDescriptorWriter::overwrite(VkDescriptorSet& set)
    {
        for (auto& write : writes)
        {
            write.dstSet = set;
        }
        // vkUpdateDescriptorSets(pool.vtDevice.device(), writes.size(), writes.data(), 0, nullptr);
        vkUpdateDescriptorSets(pool.vtDevice.device(), (uint32_t)writes.size(), writes.data(), 0, nullptr);
    }

}