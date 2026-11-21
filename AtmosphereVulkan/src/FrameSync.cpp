#include "FrameSync.h"
#include <iostream>
#include <stdexcept>

FrameSync::FrameSync(VkDevice device, uint32_t framesInFlight, uint32_t swapchainImageCount)
{
    this->device = device;

    //==========================================================
    // creating a fence to tell the CPU that the GPU finished working
    //==========================================================
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    inFlightFences.resize( framesInFlight );
    for (size_t i = 0; i < framesInFlight; i++)
    {
        inFlightFences[i] = VK_NULL_HANDLE;
    }

    VkResult result;
    for (size_t i = 0; i < framesInFlight; i++)
    {
        result = vkCreateFence(
            device,
            &fenceInfo,
            nullptr,
            &inFlightFences[i]
        );

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("\033[31m Failed to create fence!\n");
        }
    }

    std::cout << "\033[32m In Flight Fences created successfully \033[0m\n";

    imagesInFlight.resize(swapchainImageCount);
    for (size_t i = 0; i < swapchainImageCount; i++)
    {
        imagesInFlight[i] = VK_NULL_HANDLE; //No previous frame is currently associated with this image
    }

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    imageAvailable.resize(framesInFlight, VK_NULL_HANDLE);
    renderFinished.resize(framesInFlight, VK_NULL_HANDLE);

    for (uint32_t i = 0; i < framesInFlight; i++)
    {
        VkResult result = vkCreateSemaphore(
            device,
            &semaphoreInfo,
            nullptr,
            &imageAvailable[i]
        );

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image-available semaphore!");
        }

        result = vkCreateSemaphore(
            device,
            &semaphoreInfo,
            nullptr,
            &renderFinished[i]
        );

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render-finished semaphore!");
        }
    }
}

FrameSync::~FrameSync()
{
    for (size_t i = 0; i < inFlightFences.size(); i++)
    {
        vkDestroyFence(device, inFlightFences[i], nullptr);
        std::cout << "fence " << i << " destroyed.\n";
    }

    for (VkSemaphore semaphore : imageAvailable)
    {
        if (semaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(device, semaphore, nullptr);
    }

    for (VkSemaphore semaphore : renderFinished)
    {
        if (semaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(device, semaphore, nullptr);
    }

    std::cout << "Semaphores destroyed.\n";
}

VkSemaphore FrameSync::getImageAvailable(uint32_t frameIndex) const 
{
    return imageAvailable[frameIndex];
}

// Returns the fence associated with the current CPU frame slot (for vkWaitForFences)
VkFence FrameSync::getInFlightFence(uint32_t frameIndex) const {
    return inFlightFences[frameIndex];
}

// Returns the fence currently holding up a specific swapchain image
VkFence FrameSync::getImagesInFlight(uint32_t frameIndex) const {
    return imagesInFlight[frameIndex];
}

VkSemaphore FrameSync::getRenderFinished(uint32_t frameIndex) const {
    return renderFinished[frameIndex];
}

// Associates a swapchain image with the fence of the frame that is currently rendering to it
void FrameSync::setImagesInFlight(uint32_t imageIndex, uint32_t currentFrame) {
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];
}
