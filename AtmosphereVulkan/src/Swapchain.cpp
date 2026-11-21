#include "Swapchain.h"
#include "VulkanSurface.h"
#include <iostream>
#include <stdexcept>

Swapchain::Swapchain(const VulkanSurface& vkSurface, VkDevice device) : device(device)
{

    //==========================================================
    // create the swapchain info using the data we got before
    //==========================================================
    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = vkSurface.getSurface();
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageExtent = vkSurface.getExtent(); // This specifies the width and height of each swapchain image.
    swapchainInfo.imageFormat = vkSurface.getFormat().format; // This determines how the image stores its color information.
    swapchainInfo.imageColorSpace = vkSurface.getFormat().colorSpace;
    swapchainInfo.imageArrayLayers = imageArrayLayers; //how many image layers each swapchain image has.
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT; //What kinds of operations is this image allowed to be used for?
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //This image is owned by one queue family at a time.
    swapchainInfo.preTransform = vkSurface.getCapabilities().currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = vkSurface.getPresentMode();
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(
        device,
        &swapchainInfo,
        nullptr,
        &swapchain
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31mFailed to create swapchain!\n");
        
    }
    std::cout << "\033[32m Swapchain created successfully! \033[0m\n";

    //==========================================================
    // now we get the images from the swapchain so we can render on them
    //==========================================================

    // so first we get how many images there are in the swapchain
    result = vkGetSwapchainImagesKHR(
        device,
        swapchain,
        &swapchainImageCount,
        nullptr
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31m Failed to get swapchain image count!\n");
    }

    std::cout << "Number of swapchain images: "
        << swapchainImageCount
        << "\n";

    swapchainImages.resize(swapchainImageCount); // We allocate enough space in our vector.

    //now we create a vector with imageCount and get the images
    result = vkGetSwapchainImagesKHR(
        device,
        swapchain,
        &swapchainImageCount,
        swapchainImages.data()
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31mFailed to get swapchain images!\n");
    }

    // creating a image layout vector to save the previos layouts
    swapchainImageLayouts.resize(
        swapchainImageCount,
        VK_IMAGE_LAYOUT_UNDEFINED
    );

    // print the images
    // NOTE: we requested "swapchainInfo.minImageCount = 2;" but we might get more images
    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        std::cout << "Swapchain image "
            << i
            << ": "
            << swapchainImages[i]
            << "\n";
    }

    //==========================================================
    // Now we need to create an image view for each swapchain image.
    // VkImage is the actual image resource, while VkImageView describes how Vulkan should access/interpret that image.
    //==========================================================

    swapchainImageViews.resize(
        swapchainImages.size(),
        VK_NULL_HANDLE
    );

    for (size_t i = 0; i < swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo viewInfo{};

        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        viewInfo.image = swapchainImages[i]; // The swapchain image this view refers to.
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // We want to view it as a 2D image.
        viewInfo.format = vkSurface.getFormat().format; // The image's pixel format, matching our swapchain.

        // The component swizzles are all IDENTITY, meaning red stays red, green stays green, and so on.
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // this is a color image, not a depth image.

        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(
            device,
            &viewInfo,
            nullptr,
            &swapchainImageViews[i]
        );

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("\033[31mFailed to create image view \n");
        }

        std::cout << "Created image view " << i << "\n";
    }

}

Swapchain::~Swapchain()
{
    // 1. Destroy image views first
    for (VkImageView imageView : swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(
                device,
                imageView,
                nullptr
            );
        }
    }

    swapchainImageViews.clear();

    std::cout << "Image views destroyed.\n";

    // 2. Destroy the swapchain afterwards
    if (swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(
            device,
            swapchain,
            nullptr
        );

        swapchain = VK_NULL_HANDLE;
    }
}

void Swapchain::cleanup()
{
    for (auto imageView : swapchainImageViews)
    {
        if (imageView != VK_NULL_HANDLE)
            vkDestroyImageView(device, imageView, nullptr);
    }

    swapchainImageViews.clear();
    swapchainImages.clear();
    swapchainImageLayouts.clear();

    if (swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }
}

void Swapchain::recreate(const VulkanSurface& vkSurface, VkDevice device)
{
    
    cleanup();
    this->device = device;

    //==========================================================
    // create the swapchain info using the data we got before
    //==========================================================
    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = vkSurface.getSurface();
    swapchainInfo.minImageCount = imageCount;
    swapchainInfo.imageExtent = vkSurface.getExtent();
    swapchainInfo.imageFormat = vkSurface.getFormat().format;
    swapchainInfo.imageColorSpace = vkSurface.getFormat().colorSpace;
    swapchainInfo.imageArrayLayers = imageArrayLayers; //how many image layers each swapchain image has.
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; //What kinds of operations is this image allowed to be used for?
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //This image is owned by one queue family at a time.
    swapchainInfo.preTransform = vkSurface.getCapabilities().currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = vkSurface.getPresentMode();
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(
        device,
        &swapchainInfo,
        nullptr,
        &swapchain
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31mFailed to create swapchain!\n");

    }
    std::cout << "\033[32m Swapchain created successfully! \033[0m\n";

    //==========================================================
    // now we get the images from the swapchain so we can render on them
    //==========================================================

    // so first we get how many images there are in the swapchain
    result = vkGetSwapchainImagesKHR(
        device,
        swapchain,
        &swapchainImageCount,
        nullptr
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31m Failed to get swapchain image count!\n");
    }

    std::cout << "Number of swapchain images: "
        << swapchainImageCount
        << "\n";

    //now we create a vector with imageCount and get the images
    swapchainImages.resize(swapchainImageCount);
    result = vkGetSwapchainImagesKHR(
        device,
        swapchain,
        &swapchainImageCount,
        swapchainImages.data()
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31mFailed to get swapchain images!\n");
    }

    // creating a image layout vector to save the previos layouts
    swapchainImageLayouts.resize(
        swapchainImageCount,
        VK_IMAGE_LAYOUT_UNDEFINED
    );

    // print the images
    // NOTE: we requested "swapchainInfo.minImageCount = 2;" but we might get more images
    for (uint32_t i = 0; i < swapchainImageCount; i++)
    {
        std::cout << "Swapchain image "
            << i
            << ": "
            << swapchainImages[i]
            << "\n";
    }

    //==========================================================
    // Now we need to create an image view for each swapchain image.
    // VkImage is the actual image resource, while VkImageView describes how Vulkan should access/interpret that image.
    //==========================================================

    swapchainImageViews.resize(
        swapchainImages.size(),
        VK_NULL_HANDLE
    );

    for (size_t i = 0; i < swapchainImages.size(); i++)
    {
        VkImageViewCreateInfo viewInfo{};

        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

        viewInfo.image = swapchainImages[i]; // The swapchain image this view refers to.
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // We want to view it as a 2D image.
        viewInfo.format = vkSurface.getFormat().format; // The image's pixel format, matching our swapchain.

        // The component swizzles are all IDENTITY, meaning red stays red, green stays green, and so on.
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // this is a color image, not a depth image.

        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        result = vkCreateImageView(
            device,
            &viewInfo,
            nullptr,
            &swapchainImageViews[i]
        );

        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("\033[31mFailed to create image view \n");
        }

        std::cout << "Created image view " << i << "\n";
    }
}