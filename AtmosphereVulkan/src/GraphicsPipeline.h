#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

struct GraphicsPipelineConfig
{
    VkPrimitiveTopology topology;

    VkBool32 depthWriteEnable;

    VkBool32 blendingEnable;
};

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
            VkFormat depthFormat, // Default value added here
            VkDescriptorSetLayout cameraDescriptorSetLayout,
            const GraphicsPipelineConfig& config
        );

        ~GraphicsPipeline();

        void bind(VkCommandBuffer commandBuffer) const;

        VkPipeline getPipeline() const;
        VkPipelineLayout getPipelineLayout() const { return pipelineLayout; };

        void destroy();

    private:
        VkDevice device = VK_NULL_HANDLE;

        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

        std::vector<char> readFileVertex(const std::string& filename);
};

