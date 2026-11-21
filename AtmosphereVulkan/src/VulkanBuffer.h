#pragma once
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>

// Simplified usage flags
enum class BufferType {
    Storage,      // Compute / SSBO read/write
    Vertex,       // Geometry vertex data
    Index,        // Triangle indices
    Uniform,      // Read-only shader constants
    StagingSrc,   // Transfer source (CPU -> GPU staging)
    StagingDst    // Transfer destination (GPU -> CPU readback)
};

// Simplified memory locations
enum class MemoryType {
    CpuToGpu,     // Host Visible + Coherent (CPU can write directly via memcpy)
    GpuOnly,      // Fast VRAM (GPU only, requires staging buffer to write)
    GpuToCpu      // Host Visible + Cached (CPU can read back results fast)
};

class VulkanBuffer
{
    private:
        VkDevice device{ VK_NULL_HANDLE };
        VkBuffer buffer{ VK_NULL_HANDLE };
        VkDeviceMemory memory{ VK_NULL_HANDLE };
        VkDeviceSize size{ 0 };
        void* mappedPtr = nullptr;
        
    public:
        // Flexible Constructor with default parameters for sharing mode
        VulkanBuffer(
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize size,
            BufferType type,
            MemoryType memoryLocation,
            VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            const std::vector<uint32_t>& queueFamilyIndices = {}
        );

        // New constructor taking raw VkBufferUsageFlags directly
        VulkanBuffer(
            VkDevice device,
            VkPhysicalDevice physicalDevice,
            VkDeviceSize size,
            VkBufferUsageFlags usageFlags,
            MemoryType memoryLocation,
            VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            const std::vector<uint32_t>& queueFamilyIndices = {}
        );

        ~VulkanBuffer();

        // Prevent accidental copies (avoids double-free crashes)
        VulkanBuffer(const VulkanBuffer&) = delete;
        VulkanBuffer& operator=(const VulkanBuffer&) = delete;

        // Support Move Semantics (allows storing VulkanBuffer inside std::vector)
        VulkanBuffer(VulkanBuffer&& other) noexcept;
        VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

        // Getters for Vulkan calls
        VkBuffer getBuffer() const { return buffer; }
        VkDeviceMemory getMemory() const { return memory; }
        VkDeviceSize getSize() const { return size; }
        void* getMappedPtr() const noexcept { return mappedPtr; }
        
        // Helper to find GPU memory type (Host Visible vs. Device Local)
        static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);


        void copyTo(const void* data, VkDeviceSize dataSize, VkDeviceSize offset = 0);

        // Helper overload for std::vector

        /// @brief Copies a std::vector into host-visible GPU memory.
        /// @param vec Vector passed by const reference (prevents heavy memory copies and guarantees read-only access).
        /// @param offset Starting byte position in the GPU buffer (defaults to 0).
        /// @return void (Does not return a value).
        template<typename T>
        void copyVector(const std::vector<T>& vec, VkDeviceSize offset = 0) {
            copyTo(vec.data(), vec.size() * sizeof(T), offset);
        }

        
};

