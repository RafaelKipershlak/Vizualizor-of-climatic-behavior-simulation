#pragma once
#include <string>
#include <vulkan/vulkan.h>


class Texture
{
    private:
    VkDevice device = VK_NULL_HANDLE;
    VkImage textureImage = VK_NULL_HANDLE;
    VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    VkImageView textureImageView = VK_NULL_HANDLE;
    VkSampler textureSampler = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;

    // Internal setup steps (implemented in Texture.cpp)
    void createTextureImage(
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const std::string& texturePath
    );

    void createTextureImageView();
    void createTextureSampler(VkPhysicalDevice physicalDevice);
    void cleanup();

public:
    Texture(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        const std::string& texturePath
    ); 

    ~Texture();

    // Prevent accidental copying (would double-free GPU image/memory)
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Allow safe moving
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    // getters
    VkImageView getImageView() const { return textureImageView; }
    VkSampler getSampler() const { return textureSampler; }
    uint32_t getWidth() const { return width; }
    uint32_t getHeight() const { return height; }
};