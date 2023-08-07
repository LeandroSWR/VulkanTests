#pragma once

#include "vt_device.hpp"

// std
#include <string.h>

namespace vt 
{
	class Texture
	{
	public:
		static constexpr bool USE_SRGB = true;
		static constexpr bool USE_UNORM = false;
	public:
		Texture(VtDevice &device, const std::string& filepath, bool sRGB);
		~Texture();

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
		Texture(Texture&&) = delete;
		Texture& operator=(Texture&&) = delete;

		VkSampler getSampler() { return sampler; }
		VkImageView getImageView() { return imageView; }
		VkImageLayout getImageLayout() { return imageLayout; }

		VkDescriptorImageInfo getDescriptorImageInfo();

	private:
		void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
		void generateMipmaps();

		int width, height, mipLevels;

		VtDevice& vtDevice;
		VkImage image;
		VkDeviceMemory imageMemory;
		VkImageView imageView;
		VkSampler sampler;
		VkFormat imageFormat;
		VkImageLayout imageLayout;
	};
}