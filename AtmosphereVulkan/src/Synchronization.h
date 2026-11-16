#pragma once

#include <vulkan/vulkan.h>

namespace Sync
{
    void bufferBarrier(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkPipelineStageFlags2 srcStage,
        VkAccessFlags2 srcAccess,
        VkPipelineStageFlags2 dstStage,
        VkAccessFlags2 dstAccess,
        VkDeviceSize offset,
        VkDeviceSize size
    );

    void imageMemoryBarrier(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask,
        VkAccessFlags2 srcAccessMask,
        VkPipelineStageFlags2 dstStageMask,
        VkAccessFlags2 dstAccessMask
    );
}