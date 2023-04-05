#pragma once

#include <vt_device.hpp>

// std
#include <string.h>

namespace vt 
{
	class Texture
	{
	public:
		Texture(VtDevice &device, const std::string& path);
		~Texture();

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
		Texture(Texture&&) = delete;
		Texture& operator=(Texture&&) = delete;

		VkSampler getSampler() { return sampler; }
		VkImageView getImageView() { return imageView; }
		VkImageLayout getImageLayout() { return imageLayout; }

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