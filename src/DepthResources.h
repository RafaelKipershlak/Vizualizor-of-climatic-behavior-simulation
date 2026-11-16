#include <iostream>
#include <vulkan/vulkan.h>
#include "VulkanSetup.h"
#include "VulkanBuffer.h"
#include <vector>
#include <chrono>
#include <fstream>
#include <stdexcept>
#include "ComputeShaders.h"
#include "GraphicsPipeline.h"
#include "Vertex.h"
#include "Synchronization.h"
#include "VulkanSurface.h"
#include "Swapchain.h"
#include "FrameSync.h"
#include <GLFW/glfw3.h>

#pragma once
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
