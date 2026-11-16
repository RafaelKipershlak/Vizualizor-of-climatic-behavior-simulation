#include "DepthResources.h"

VkFormat DepthResources::findSupportedDepthFormat(VkPhysicalDevice physicalDevice)
{
    std::vector<VkFormat> candidates =
    {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_D16_UNORM
    };

    for (VkFormat format : candidates)
    {
        VkFormatProperties properties{};

        vkGetPhysicalDeviceFormatProperties(
            physicalDevice,
            format,
            &properties
        );

        if (properties.optimalTilingFeatures &
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
        {
            return format;
        }
    }

    throw std::runtime_error(
        "Failed to find a supported depth format!"
    );
}

DepthResources::DepthResources(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkExtent2D extent)
{
    this->device = device;
    depthFormat = findSupportedDepthFormat(physicalDevice);

    VkImageCreateInfo imageInfo{};
    
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D; 
    
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.extent.depth = 1;

    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;

    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &depthImage) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create depth image!");
    }

    //allocate memory for the image

    //first we ask vulkan how much memory the image requires
    VkMemoryRequirements memRequirements{};

    vkGetImageMemoryRequirements(
        device,
        depthImage,
        &memRequirements
    );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

    allocInfo.allocationSize = memRequirements.size;

    //DEVICE_LOCAL Because the depth image is used during rendering by the GPU. We generally want its memory to be accessible efficiently by the GPU.
    allocInfo.memoryTypeIndex = VulkanBuffer::findMemoryType(physicalDevice, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (vkAllocateMemory(
        device,
        &allocInfo,
        nullptr,
        &depthMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate depth image memory!");
    }

    // now we bind the memory to our image
    if (vkBindImageMemory(
        device,
        depthImage,
        depthMemory,
        0) != VK_SUCCESS) //The 0 is the offset inside the allocated memory.
    {
        throw std::runtime_error("Failed to bind depth image memory!");
    }

    // now we need the imageView to tell vulkan how to access the image.
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

    viewInfo.image = depthImage;

    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // Our depth image is a 2D image, so we create a 2D view.
    
    viewInfo.format = depthFormat; // This matches the format we used when creating the image.

    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // This tells Vulkan that we want to access the depth aspect of the image.

    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;

    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(
        device,
        &viewInfo,
        nullptr,
        &depthImageView) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create depth image view!");
    }

}

DepthResources::~DepthResources()
{
    if (depthImageView != VK_NULL_HANDLE)
        vkDestroyImageView(device, depthImageView, nullptr);

    if (depthImage != VK_NULL_HANDLE)
        vkDestroyImage(device, depthImage, nullptr);

    if (depthMemory != VK_NULL_HANDLE)
        vkFreeMemory(device, depthMemory, nullptr);
}

/*
void DepthResources::transitionLayout(VkCommandBuffer commandBuffer)
{
    Sync::imageMemoryBarrier(
        commandBuffer,
        depthImage,

        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,

        VK_PIPELINE_STAGE_2_NONE,
        VK_ACCESS_2_NONE,

        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
    );
}
*/


void DepthResources::transitionLayout(VkCommandBuffer commandBuffer)
{
    VkImageMemoryBarrier2 barrier{};

    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;

    // The image has not been used before.
    barrier.srcStageMask = VK_PIPELINE_STAGE_2_NONE;
    barrier.srcAccessMask = VK_ACCESS_2_NONE;

    // We want to use it as a depth attachment.
    barrier.dstStageMask =
        VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;

    barrier.dstAccessMask =
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    barrier.image = depthImage;

    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkDependencyInfo dependency{};

    dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependency.imageMemoryBarrierCount = 1;
    dependency.pImageMemoryBarriers = &barrier;

    vkCmdPipelineBarrier2(commandBuffer, &dependency);
}