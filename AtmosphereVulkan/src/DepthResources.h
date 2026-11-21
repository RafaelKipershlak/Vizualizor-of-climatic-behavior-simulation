#pragma once
#include <vulkan/vulkan.h>

class DepthResources
{
private:
    VkDevice device = VK_NULL_HANDLE;
    VkImage depthImage = VK_NULL_HANDLE;
    VkDeviceMemory depthMemory = VK_NULL_HANDLE;
    VkImageView depthImageView = VK_NULL_HANDLE;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    VkFormat findSupportedDepthFormat(VkPhysicalDevice physicalDevice);

public:
    DepthResources(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkExtent2D extent
    );

    ~DepthResources();

    VkImageView getImageView() const { return depthImageView; }
    VkFormat getFormat() const { return depthFormat; }

    void transitionLayout(VkCommandBuffer commandBuffer);
};
