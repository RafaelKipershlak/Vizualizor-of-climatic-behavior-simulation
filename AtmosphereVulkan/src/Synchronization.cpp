#include "Synchronization.h"

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
    )
    {
        VkBufferMemoryBarrier2 barrier{};

        barrier.sType =
            VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;

        barrier.srcStageMask = srcStage;
        barrier.srcAccessMask = srcAccess;

        barrier.dstStageMask = dstStage;
        barrier.dstAccessMask = dstAccess;

        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        barrier.buffer = buffer;
        barrier.offset = offset;
        barrier.size = size;

        VkDependencyInfo dependencyInfo{};

        dependencyInfo.sType =
            VK_STRUCTURE_TYPE_DEPENDENCY_INFO;

        dependencyInfo.bufferMemoryBarrierCount = 1;
        dependencyInfo.pBufferMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(
            commandBuffer,
            &dependencyInfo
        );
    }

    void imageMemoryBarrier(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout,
        VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask,
        VkAccessFlags2 srcAccessMask,
        VkPipelineStageFlags2 dstStageMask,
        VkAccessFlags2 dstAccessMask)
    {
        VkImageMemoryBarrier2 imageMemBarrier{};
        imageMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        imageMemBarrier.image = image;

        imageMemBarrier.srcStageMask = srcStageMask;
        imageMemBarrier.srcAccessMask = srcAccessMask;

        imageMemBarrier.dstStageMask = dstStageMask;
        imageMemBarrier.dstAccessMask = dstAccessMask;

        imageMemBarrier.oldLayout = oldLayout;
        imageMemBarrier.newLayout = newLayout;

        imageMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        imageMemBarrier.subresourceRange.baseMipLevel = 0;
        imageMemBarrier.subresourceRange.levelCount = 1;

        imageMemBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemBarrier.subresourceRange.layerCount = 1;

        imageMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        VkDependencyInfo dependencyInfo{};

        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &imageMemBarrier;

        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo); // prepare image
    }
}