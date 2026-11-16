#pragma once
#include <string>
#include <vulkan/vulkan.h>
#include <cstdint> // For uint32_t
#include <vector>
#include <iostream>

class VulkanSetup
{
    private:
        VkInstance instance{ VK_NULL_HANDLE };
        uint32_t deviceCount = 0;
        VkPhysicalDevice physicalGPU{ VK_NULL_HANDLE };
        uint32_t queueFamilyCount = 0;

        uint32_t graphicsFamily = UINT32_MAX;
        uint32_t computeFamily  = UINT32_MAX;
        uint32_t transferFamily = UINT32_MAX;

        VkDevice device{ VK_NULL_HANDLE };
        VkQueue graphicsQueue{ VK_NULL_HANDLE };
        VkQueue computeQueue{ VK_NULL_HANDLE };
        VkQueue transferQueue{ VK_NULL_HANDLE };

        VkCommandPool commandPool{ VK_NULL_HANDLE };
        VkCommandPool transferCommandPool{ VK_NULL_HANDLE };

        // Multi-command buffer management
        std::vector<VkCommandBuffer> commandBuffers;
        uint32_t activeIndex{ 0 };

    public:
        VulkanSetup(uint32_t extensionCount = 0, const char** extensions = nullptr); // Constructor
        ~VulkanSetup(); // Destructor to destroy instance

        // Deleted copy constructors to prevent handle duplication
        VulkanSetup(const VulkanSetup&) = delete;
        VulkanSetup& operator=(const VulkanSetup&) = delete;

        // Allocate N command buffers (e.g., up to 3)
        bool createCommandBuffers(uint32_t count = 3);

        // Command Recording API
        bool StartRecording(uint32_t bufferIndex = 0);
        bool StopRecording(uint32_t bufferIndex = 0);

        void submitCommandBuffer(
            uint32_t bufferIndex = 0,
            VkSemaphore waitSemaphore = VK_NULL_HANDLE,
            VkSemaphore signalSemaphore = VK_NULL_HANDLE,
            VkFence fence= VK_NULL_HANDLE
        );

        // =====================================================
        // Getter Functions
        // =====================================================

        // Vulkan Handles
        VkInstance getInstance() const noexcept { return instance; }
        VkPhysicalDevice getPhysicalDevice() const noexcept { return physicalGPU; }
        VkDevice getDevice() const noexcept { return device; }

        VkQueue getGraphicsQueue() const noexcept { return graphicsQueue; }
        VkQueue getComputeQueue() const noexcept { return computeQueue; }
        VkQueue getTransferQueue() const noexcept { return transferQueue; }

        VkCommandPool getCommandPool() const noexcept { return commandPool; }
        VkCommandPool getTransferCommandPool() const noexcept { return transferCommandPool; }
        // Access specific command buffer
        VkCommandBuffer getCommandBuffer(uint32_t index = 0) const noexcept {
            if (index < commandBuffers.size()) {
                return commandBuffers[index];
            }
            std::cout << "Error: Command buffer index out of bounds!\n";
            return VK_NULL_HANDLE;
        }

        // Metadata & Queue Indices
        uint32_t getGraphicsFamily() const noexcept { return graphicsFamily; }
        uint32_t getComputeFamily() const noexcept { return computeFamily; }
        uint32_t getTransferFamily() const noexcept { return transferFamily; }
        uint32_t getDeviceCount() const noexcept { return deviceCount; }
        uint32_t getQueueFamilyCount() const noexcept { return queueFamilyCount; }
        uint32_t getCommandBuffersCount() const noexcept { return commandBuffers.size(); }
        
};

