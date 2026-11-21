#include "VulkanSurface.h"
#include <algorithm>
#include <cstdint>
#include <iostream>

VkPresentModeKHR VulkanSurface::choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
    for (const auto& mode : presentModes)
    {
        if (mode == VK_PRESENT_MODE_FIFO_KHR)
        {
            return VK_PRESENT_MODE_FIFO_KHR;
        }
    }

    return VK_PRESENT_MODE_IMMEDIATE_KHR;
}

const char* VulkanSurface::formatToString(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_B8G8R8A8_SRGB:
        return "VK_FORMAT_B8G8R8A8_SRGB";

    case VK_FORMAT_B8G8R8A8_UNORM:
        return "VK_FORMAT_B8G8R8A8_UNORM";

    case VK_FORMAT_R8G8B8A8_UNORM:
        return "VK_FORMAT_R8G8B8A8_UNORM";

    case VK_FORMAT_R8G8B8A8_SRGB:
        return "VK_FORMAT_R8G8B8A8_SRGB";

    default:
        return "Unknown";
    }
}

VkSurfaceFormatKHR  VulkanSurface::chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    for (size_t i = 0; i < formats.size(); i++)
    {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return formats[i];
        }
    }

    // Fallback
    return formats[0];
}

VulkanSurface::VulkanSurface(VkInstance instance, GLFWwindow* window, VkPhysicalDevice physicalDevice) : instance(instance), physicalDevice(physicalDevice), window(window)
{
    //==========================================================
    //create vulkan surface
    //==========================================================

    VkResult result = glfwCreateWindowSurface(
        instance,
        window,
        nullptr,
        &surface
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31m Failed to create Vulkan surface!\n");
    }

    std::cout << "\033[32m Vulkan surface created successfully! \033[0m \n";

    //==========================================================
    // check if our queue can display 
    // A queue that supports graphics doesn't necessarily support presenting images to our window.
    // That's why we check presentation support.
    //==========================================================

    // One thing to improve later: you're using queue family index 0 directly.
    // We should use the actual queue family selected by VulkanSetup, rather than assuming it's always zero.

    result = vkGetPhysicalDeviceSurfaceSupportKHR(
        physicalDevice,
        0,
        surface,
        &presentSupport
    );

    std::cout << "Present: " << (presentSupport ? "YES" : "NO") << "\n";
    if (presentSupport == VK_FALSE)
    {
        throw std::runtime_error("\033[31m Your device can't present!\nPlease choose another device");
    }

    //==========================================================
    //check surface capabilities
    //==========================================================
    

    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice,
        surface,
        &capabilities
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31m Failed to get surface capabilities!\n");
    }

    std::cout << "Surface capabilities:\n";
    std::cout << "  Min image count: "
        << capabilities.minImageCount << "\n";

    std::cout << "  Max image count: "
        << capabilities.maxImageCount << "\n";

    std::cout << "  Current extent: "
        << capabilities.currentExtent.width
        << " x "
        << capabilities.currentExtent.height
        << "\n";

    //A format describes how image color data is stored.
    // example: VK_FORMAT_B8G8R8A8_SRGB means , 8 bits blue, 8 bits green, 8 bits red, 8 bits alpha

    uint32_t formatCount = 0;

    result =
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &formatCount,
            nullptr
        );

    std::cout << "Number of surface formats: " << formatCount << "\n";

    std::vector<VkSurfaceFormatKHR> formats(formatCount);

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface,
        &formatCount,
        formats.data()
    );

    for (const auto& format : formats)
    {
        std::cout
            << "Format: " << format.format
            << " | Color space: " << format.colorSpace
            << "\n";
    }

    //choose surface format 
    surfaceFormat = chooseSurfaceFormat(formats);
    std::cout << "Selected format: " << formatToString(surfaceFormat.format) << "\n";
    std::cout << "Selected color space: " << surfaceFormat.colorSpace << "\n";

    //print all supported present models
    uint32_t presentModeCount;

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice,
        surface,
        &presentModeCount,
        nullptr
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31m Failed to get present modes!\n");
    }

    //Present modes
    //This determines how rendered images are presented.
    //example, FIFO : Images are presented in order, synchronized with the display refresh cycle.

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice,
        surface,
        &presentModeCount,
        presentModes.data()
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31m Failed to get present modes!\n");
    }

    for (const auto& mode : presentModes)
    {
        std::cout
            << "Present Mode : " << mode
            << "\n";
    }

    presentMode = choosePresentMode(presentModes);

    if (presentMode == VK_PRESENT_MODE_FIFO_KHR)
    {
        std::cout << "Selected: FIFO\n";
    }
    else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
    {
        std::cout << "Selected: IMMEDIATE\n";
    }

    // Surface extent
    // this describes in what dementions the surface is presented, for example 1280x720

    extent = capabilities.currentExtent;

    std::cout
        << "Selected swapchain extent: "
        << extent.width
        << " x "
        << extent.height
        << "\n";

    std::cout << "Swapchain format: "
        << surfaceFormat.format
        << "\n";

    std::cout << "Swapchain color space: "
        << surfaceFormat.colorSpace
        << "\n";

    if (capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        std::cout << "Color attachment usage supported!\n";
    }
    else
    {
        std::cout << "Color attachment usage NOT supported!\n";
    }

    std::cout << "Swapchain sharing mode: EXCLUSIVE\n";

    std::cout
        << "Current surface transform: "
        << capabilities.currentTransform
        << "\n";

    if (capabilities.supportedTransforms &
        capabilities.currentTransform)
    {
        std::cout << "Current transform is supported!\n";
    }
    else
    {
        std::cout << "Current transform is NOT supported!\n";
    }

    if (capabilities.supportedCompositeAlpha &
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    {
        std::cout << "Opaque composite alpha supported!\n";
    }
    else
    {
        std::cout << "Opaque composite alpha NOT supported!\n";
    }

    std::cout << "Swapchain present mode selected.\n";

}

VulkanSurface::~VulkanSurface()
{
    if (surface != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance, surface, nullptr);
        surface = VK_NULL_HANDLE;
    }
}

void VulkanSurface::refresh()
{
    //==========================================================
    //check surface capabilities
    //==========================================================


    VkResult result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        physicalDevice,
        surface,
        &capabilities
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31m Failed to get surface capabilities!\n");
    }

    std::cout << "Surface capabilities:\n";
    std::cout << "  Min image count: "
        << capabilities.minImageCount << "\n";

    std::cout << "  Max image count: "
        << capabilities.maxImageCount << "\n";

    std::cout << "  Current extent: "
        << capabilities.currentExtent.width
        << " x "
        << capabilities.currentExtent.height
        << "\n";

    uint32_t formatCount = 0;

    result =
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            physicalDevice,
            surface,
            &formatCount,
            nullptr
        );

    std::cout << "Number of surface formats: " << formatCount << "\n";

    std::vector<VkSurfaceFormatKHR> formats(formatCount);

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        physicalDevice,
        surface,
        &formatCount,
        formats.data()
    );

    for (const auto& format : formats)
    {
        std::cout
            << "Format: " << format.format
            << " | Color space: " << format.colorSpace
            << "\n";
    }

    //choose surface format 
    surfaceFormat = chooseSurfaceFormat(formats);
    std::cout << "Selected format: " << formatToString(surfaceFormat.format) << "\n";
    std::cout << "Selected color space: " << surfaceFormat.colorSpace << "\n";

    //print all supported present models
    uint32_t presentModeCount;

    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice,
        surface,
        &presentModeCount,
        nullptr
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31m Failed to get present modes!\n");
    }

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        physicalDevice,
        surface,
        &presentModeCount,
        presentModes.data()
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31m Failed to get present modes!\n");
    }

    for (const auto& mode : presentModes)
    {
        std::cout
            << "Present Mode : " << mode
            << "\n";
    }

    presentMode = choosePresentMode(presentModes);

    if (presentMode == VK_PRESENT_MODE_FIFO_KHR)
    {
        std::cout << "Selected: FIFO\n";
    }
    else if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
    {
        std::cout << "Selected: IMMEDIATE\n";
    }

    extent = capabilities.currentExtent;

    std::cout
        << "Selected swapchain extent: "
        << extent.width
        << " x "
        << extent.height
        << "\n";

    std::cout << "Swapchain format: "
        << surfaceFormat.format
        << "\n";

    std::cout << "Swapchain color space: "
        << surfaceFormat.colorSpace
        << "\n";

    if (capabilities.supportedUsageFlags &
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
    {
        std::cout << "Color attachment usage supported!\n";
    }
    else
    {
        std::cout << "Color attachment usage NOT supported!\n";
    }

    std::cout << "Swapchain sharing mode: EXCLUSIVE\n";

    std::cout
        << "Current surface transform: "
        << capabilities.currentTransform
        << "\n";

    if (capabilities.supportedTransforms &
        capabilities.currentTransform)
    {
        std::cout << "Current transform is supported!\n";
    }
    else
    {
        std::cout << "Current transform is NOT supported!\n";
    }

    if (capabilities.supportedCompositeAlpha &
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    {
        std::cout << "Opaque composite alpha supported!\n";
    }
    else
    {
        std::cout << "Opaque composite alpha NOT supported!\n";
    }

    std::cout << "Swapchain present mode selected.\n";
}