#pragma once

#include "vt_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace vt
{
    class VtDescriptorSetLayout
    {
    public:
        class Builder
        {
        public:
            Builder(VtDevice& vtDevice) : vtDevice{ vtDevice } {}

            Builder& addBinding(
                uint32_t binding,
                VkDescriptorType descriptorType,
                VkShaderStageFlags stageFlags,
                uint32_t count = 1);
            std::unique_ptr<VtDescriptorSetLayout> build() const;

        private:
            VtDevice& vtDevice;
            std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
        };

        VtDescriptorSetLayout(
            VtDevice& vtDevice, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
        ~VtDescriptorSetLayout();
        VtDescriptorSetLayout(const VtDescriptorSetLayout&) = delete;
        VtDescriptorSetLayout& operator=(const VtDescriptorSetLayout&) = delete;

        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }

    private:
        VtDevice& vtDevice;
        VkDescriptorSetLayout descriptorSetLayout;
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

        friend class VtDescriptorWriter;
    };

    class VtDescriptorPool
    {
    public:
        class Builder
        {
        public:
            Builder(VtDevice& vtDevice) : vtDevice{ vtDevice } {}

            Builder& addPoolSize(VkDescriptorType descriptorType, uint32_t count);
            Builder& setPoolFlags(VkDescriptorPoolCreateFlags flags);
            Builder& setMaxSets(uint32_t count);
            std::unique_ptr<VtDescriptorPool> build() const;

        private:
            VtDevice& vtDevice;
            std::vector<VkDescriptorPoolSize> poolSizes{};
            uint32_t maxSets = 1000;
            VkDescriptorPoolCreateFlags poolFlags = 0;
        };

        VtDescriptorPool(
            VtDevice& vtDevice,
            uint32_t maxSets,
            VkDescriptorPoolCreateFlags poolFlags,
            const std::vector<VkDescriptorPoolSize>& poolSizes);
        ~VtDescriptorPool();
        VtDescriptorPool(const VtDescriptorPool&) = delete;
        VtDescriptorPool& operator=(const VtDescriptorPool&) = delete;

        bool allocateDescriptorSet(
            const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const;

        void freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const;

        void resetPool();

    private:
        VtDevice& vtDevice;
        VkDescriptorPool descriptorPool;

        friend class VtDescriptorWriter;
    };

    class VtDescriptorWriter
    {
    public:
        VtDescriptorWriter(VtDescriptorSetLayout& setLayout, VtDescriptorPool& pool);

        VtDescriptorWriter& writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo);
        VtDescriptorWriter& writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo);

        bool build(VkDescriptorSet& set);
        void overwrite(VkDescriptorSet& set);

    private:
        VtDescriptorSetLayout& setLayout;
        VtDescriptorPool& pool;
        std::vector<VkWriteDescriptorSet> writes;
    };
}