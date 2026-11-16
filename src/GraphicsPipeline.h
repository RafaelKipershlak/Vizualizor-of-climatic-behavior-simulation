#pragma once
#include <iostream>
#include <vulkan/vulkan.h>
#include "VulkanSetup.h"
#include "VulkanBuffer.h"
#include <vector>
#include <chrono>
#include <fstream>
#include <stdexcept>

class GraphicsPipeline
{
    public:
        GraphicsPipeline(
            VkDevice device,
            VkFormat colorFormat,
            VkExtent2D extent,
            const std::vector<VkVertexInputBindingDescription>& bindings,
            const std::vector<VkVertexInputAttributeDescription>& attributes,
            const std::string& vertFile,
            const std::string& fragFile,
            VkFormat depthFormat = VK_FORMAT_UNDEFINED // Default value added here
        );

        ~GraphicsPipeline();

        void bind(VkCommandBuffer commandBuffer) const;

        VkPipeline getPipeline() const;
        VkPipelineLayout getPipelineLayout() const { return pipelineLayout; };

    private:
        VkDevice device = VK_NULL_HANDLE;

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

        std::vector<char> readFileVertex(const std::string& filename);
};

