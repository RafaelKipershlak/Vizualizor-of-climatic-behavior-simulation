#pragma once

#include <vulkan/vulkan.h>
#include <vector>


class FrameSync
{
public:
    FrameSync(VkDevice device, uint32_t framesInFlight, uint32_t swapchainImageCount);
    ~FrameSync();

    /// @brief Gets the image-available semaphore for the current frame slot.
    /// @param frameIndex The current frame-in-flight index.
    VkSemaphore getImageAvailable(uint32_t frameIndex) const;

    /// @brief Gets the render-finished semaphore for the current frame slot.
    /// @param frameIndex The current frame-in-flight index.
    VkSemaphore getRenderFinished(uint32_t frameIndex) const;

    /**
     * @brief Gets the fence associated with a specific CPU-GPU frame slot.
     * @param frameIndex The current frame-in-flight index (e.g., 0 or 1).
     * @return VkFence Used by the CPU (via vkWaitForFences) to wait until
     *         the GPU finishes executing this frame slot.
     */ 
    VkFence getInFlightFence(uint32_t frameIndex) const;

    /**
     * @brief Gets the fence currently tied to a specific swapchain image.
     * @param imageIndex The swapchain image index returned by vkAcquireNextImageKHR.
     * @return VkFence The fence last used to render to this swapchain image,
     *         or VK_NULL_HANDLE if no frame has used it yet.
     */
    VkFence getImagesInFlight(uint32_t imageIndex) const;

    /**
     * @brief Links a swapchain image to the fence of the frame currently drawing to it.
     * @param imageIndex The swapchain image index being rendered into.
     * @param currentFrame The current CPU frame-in-flight slot index.
     */
    void setImagesInFlight(uint32_t imageIndex, uint32_t currentFrame);

private:
    VkDevice device = VK_NULL_HANDLE;

    std::vector<VkSemaphore> imageAvailable;
    std::vector<VkSemaphore> renderFinished;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
};
