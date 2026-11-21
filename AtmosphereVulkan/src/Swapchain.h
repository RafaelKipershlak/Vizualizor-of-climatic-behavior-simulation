#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

class VulkanSurface; // Forward declaration (replaces #include "VulkanSurface.h")

class Swapchain
{
public:
	Swapchain(const VulkanSurface& vkSurface, VkDevice device);
	~Swapchain();

	uint32_t getImageCount() { return imageCount; };
	void setImageCount(uint32_t image_Count) { imageCount = image_Count; };

	uint32_t getImageArrayLayers() { return imageArrayLayers; };
	void setImageArrayLayers(uint32_t image_array_layers) { imageArrayLayers = image_array_layers; };

	uint32_t getSwapchainImageCount() { return swapchainImageCount; };

	VkSwapchainKHR getSwapchain() const { return swapchain; };
	const std::vector<VkImage>& getSwapchainImages() const
	{
		return swapchainImages;
	}

	const std::vector<VkImageView>& getSwapchainImageViews() const
	{
		return swapchainImageViews;
	}

	const std::vector<VkImageLayout>& getSwapchainImageLayouts() const
	{
		return swapchainImageLayouts;
	}

	std::vector<VkImageLayout>& getSwapchainImageLayouts()
	{
		return swapchainImageLayouts;
	}

	void recreate(const VulkanSurface& vkSurface, VkDevice device);
	void cleanup();

private:
	uint32_t imageCount = 2;
	uint32_t imageArrayLayers = 1;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkDevice device = VK_NULL_HANDLE;

	// Swapchain resources
	uint32_t swapchainImageCount = 0;
	std::vector<VkImage> swapchainImages;
	std::vector<VkImageView> swapchainImageViews;
	std::vector<VkImageLayout> swapchainImageLayouts;
};

