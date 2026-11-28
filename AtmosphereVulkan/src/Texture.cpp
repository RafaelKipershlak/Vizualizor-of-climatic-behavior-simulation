#include "Texture.h"
#include "VulkanBuffer.h"
#include "Synchronization.h"

#include <iostream>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

Texture::Texture(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    const std::string& texturePath)
    : device(device)
{
    createTextureImage(physicalDevice, commandPool, graphicsQueue, texturePath);
    createTextureImageView();
    createTextureSampler(physicalDevice);
}

Texture::~Texture()
{
    cleanup();
}

Texture::Texture(Texture&& other) noexcept
    : device(other.device),
      textureImage(other.textureImage),
      textureImageMemory(other.textureImageMemory),
      textureImageView(other.textureImageView),
      textureSampler(other.textureSampler),
      width(other.width),
      height(other.height)
{
    other.textureImage = VK_NULL_HANDLE;
    other.textureImageMemory = VK_NULL_HANDLE;
    other.textureImageView = VK_NULL_HANDLE;
    other.textureSampler = VK_NULL_HANDLE;
    other.width = 0;
    other.height = 0;
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        cleanup();

        device = other.device;
        textureImage = other.textureImage;
        textureImageMemory = other.textureImageMemory;
        textureImageView = other.textureImageView;
        textureSampler = other.textureSampler;
        width = other.width;
        height = other.height;

        other.textureImage = VK_NULL_HANDLE;
        other.textureImageMemory = VK_NULL_HANDLE;
        other.textureImageView = VK_NULL_HANDLE;
        other.textureSampler = VK_NULL_HANDLE;
        other.width = 0;
        other.height = 0;
    }
    return *this;
}

void Texture::cleanup()
{
    if (device == VK_NULL_HANDLE)
        return;

    if (textureSampler != VK_NULL_HANDLE)
        vkDestroySampler(device, textureSampler, nullptr);
        textureSampler = VK_NULL_HANDLE;

    if (textureImageView != VK_NULL_HANDLE)
        vkDestroyImageView(device, textureImageView, nullptr);
        textureImageView = VK_NULL_HANDLE;

    if (textureImage != VK_NULL_HANDLE)
        vkDestroyImage(device, textureImage, nullptr);
        textureImage = VK_NULL_HANDLE;

    if (textureImageMemory != VK_NULL_HANDLE)
        vkFreeMemory(device, textureImageMemory, nullptr);
        textureImageMemory = VK_NULL_HANDLE;
}

void Texture::createTextureImage(
    VkPhysicalDevice physicalDevice,
    VkCommandPool commandPool,
    VkQueue graphicsQueue,
    const std::string& texturePath)
{
    // 1. Decode image file into RGBA pixels on CPU
    int texWidth=0;
    int texHeight=0;
    int texChannels=0;

    stbi_uc* pixels = stbi_load(
        texturePath.c_str(),
        &texWidth,
        &texHeight,
        &texChannels,
        STBI_rgb_alpha
    );
    if (!pixels)
        throw std::runtime_error("Failed to load texture image: " + texturePath);

    
    width = static_cast<uint32_t>(texWidth);
    height = static_cast<uint32_t>(texHeight);
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(width) * height * 4; // 4 bytes per pixel (RGBA)

    // 2. Copy pixels into a CPU-to-GPU staging buffer, then free CPU pixels
    VulkanBuffer stagingBuffer(
        device,
        physicalDevice,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::CpuToGpu
    );

    stagingBuffer.copyTo(pixels, imageSize);
    stbi_image_free(pixels);

    // 3. Create DEVICE_LOCAL VkImage
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, nullptr, &textureImage) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture image!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, textureImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = VulkanBuffer::findMemoryType(
        physicalDevice,
        memRequirements.memoryTypeBits,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
    );

    if (vkAllocateMemory(device, &allocInfo, nullptr, &textureImageMemory) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate texture image memory!");

    if (vkBindImageMemory(device, textureImage, textureImageMemory, 0) != VK_SUCCESS)
        throw std::runtime_error("Failed to bind texture image memory!");

    // 4. Record & submit a one-time command buffer to upload stagingBuffer -> VkImage

    VkCommandBufferAllocateInfo allocInfoCmd{};
    allocInfoCmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfoCmd.commandPool = commandPool;
    allocInfoCmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfoCmd.commandBufferCount = 1;

    VkCommandBuffer cmdBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(device, &allocInfoCmd, &cmdBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffer for texture upload!");

    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmdBuffer, &beginInfo);

    // 4a. UNDEFINED -> TRANSFER_DST_OPTIMAL
    Sync::imageMemoryBarrier(
        cmdBuffer,
        textureImage,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_2_NONE,
        VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_COPY_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT
    );

    // 4b. Copy staging buffer into VkImage
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0; // Tightly packed
    region.bufferImageHeight = 0; // Tightly packed
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(
        cmdBuffer,
        stagingBuffer.getBuffer(),
        textureImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    // 4c. TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
    Sync::imageMemoryBarrier(
        cmdBuffer,
        textureImage,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_2_COPY_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        VK_ACCESS_2_SHADER_SAMPLED_READ_BIT
    );

    vkEndCommandBuffer(cmdBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmdBuffer;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS){
        vkFreeCommandBuffers(device, commandPool, 1, &cmdBuffer);
        throw std::runtime_error("Failed to submit command buffer for texture upload!");
    }
    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &cmdBuffer);

    std::cout << "Loaded texture (" << width << "x" << height << "): " << texturePath << "\n";
}


void Texture::createTextureImageView()
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = textureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &textureImageView) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture image view!");
}

void Texture::createTextureSampler(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;          // Seamless 360° longitude wrap
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;   // Prevent North/South pole bleeding
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE; // Disable anisotropic filtering for performance
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f; // No mipmapping for now

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS)
        throw std::runtime_error("Failed to create texture sampler!");
}