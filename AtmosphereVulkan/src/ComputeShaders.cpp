#include "ComputeShaders.h"
#include "VulkanBuffer.h" // Keep if ComputeShaders.h only forward-declares VulkanBuffer
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

std::vector<char> readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open SPV shader file: " + filename);
    }

    size_t fileSize = static_cast<size_t>(file.tellg());

    std::vector<char> buffer(fileSize);

    file.seekg(0);

    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}


ComputeShaders::ComputeShaders(
    VkDevice device,
    const std::vector<const VulkanBuffer*>& bufferPtrs,
    const std::string& spvFileName)
    : device(device)
{
    const uint32_t descriptorCount = static_cast<uint32_t>(bufferPtrs.size());

    if (descriptorCount == 0)
    {
        throw std::invalid_argument("Buffer array cannot be empty.");
    }

    // creating descriptor pool
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSize.descriptorCount = descriptorCount; // number of descreptors (3 STORAGE_BUFFER descriptors in this case)

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;

    VkResult result = vkCreateDescriptorPool(
        device,
        &poolInfo,
        nullptr,
        &descriptorPool
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Descriptor Pool.\n");
    }
    std::cout << "Descriptor Pool created successfully\n";

    // 2. Create dynamic list of descriptor set layout bindings
    std::vector<VkDescriptorSetLayoutBinding> bindings(descriptorCount);

    for (uint32_t i = 0; i < descriptorCount; ++i)
    {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        bindings[i].pImmutableSamplers = nullptr;
    }

    // create descriptor set layout

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.bindingCount = descriptorCount; // bindings number
    layoutInfo.pBindings = bindings.data();
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    result = vkCreateDescriptorSetLayout(
        device,
        &layoutInfo,
        nullptr,
        &descriptorSetLayout
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Descriptor Set Layout.\n");
    }
    std::cout << "Descriptor Set layout created successfully\n";

    // allocate (create) descriptor set
    VkDescriptorSetAllocateInfo setInfo{};
    setInfo.descriptorPool = descriptorPool;
    setInfo.descriptorSetCount = 1;
    setInfo.pSetLayouts = &descriptorSetLayout;
    setInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

    result = vkAllocateDescriptorSets(
        device,
        &setInfo,
        &descriptorSet
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Descriptor Set.");
    }
    std::cout << "Descriptor Set created successfully\n";

    //update descriptor set
    std::vector<VkDescriptorBufferInfo> bufferInfos(descriptorCount);
    std::vector<VkWriteDescriptorSet> writes(descriptorCount);
    
    for (size_t i = 0; i < descriptorCount; i++)
    {
        bufferInfos[i].buffer = bufferPtrs[i]->getBuffer();;   // Handle to the actual VkBuffer on the GPU
        bufferInfos[i].offset = 0;                          // Start reading/writing from byte offset 0
        bufferInfos[i].range = bufferPtrs[i]->getSize();      // How many bytes the shader is allowed to access

        writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;       // Required sType
        writes[i].dstSet = descriptorSet;                               // Target VkDescriptorSet handle
        writes[i].dstBinding = static_cast<uint32_t>(i);                // Targets layout(set = X, binding = 0)
        writes[i].descriptorCount = 1;                                  // Number of descriptors to update
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;   // Type of resource being written
        writes[i].pBufferInfo = &bufferInfos[i];                        // Safe: vector lives until after vkUpdateDescriptorSets
    }

    vkUpdateDescriptorSets(
        device,
        descriptorCount,
        writes.data(),
        0,
        nullptr
    );

    // create pipe line layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    result = vkCreatePipelineLayout(
        device,
        &pipelineLayoutInfo,
        nullptr,
        &pipeLineLayout
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Pipeline Layout.\n");
    }

    std::cout << "Pipeline Layout created successfully\n";

    // create shader module
    auto shaderCode = readFile(spvFileName);

    VkShaderModuleCreateInfo shaderModuleInfo{};
    shaderModuleInfo.codeSize = shaderCode.size();
    shaderModuleInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());
    shaderModuleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    VkShaderModule shaderModule{ VK_NULL_HANDLE };
    result = vkCreateShaderModule(
        device,
        &shaderModuleInfo,
        nullptr,
        &shaderModule
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Shader Module.\n");
    }

    std::cout << "Shader Module created successfully\n";


    // create compute pipeline
    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = shaderModule;
    shaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.layout = pipeLineLayout;
    computePipelineInfo.stage = shaderStageInfo;
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;

    result = vkCreateComputePipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &computePipelineInfo,
        nullptr,
        &pipeLine
    );

    // Cleanup shader module immediately after pipeline creation
    vkDestroyShaderModule(device, shaderModule, nullptr);

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Compute Pipeline.\n");
    }

    std::cout << "Compute Pipeline created successfully\n";

}

ComputeShaders::~ComputeShaders()
{
    if (pipeLine != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, pipeLine, nullptr);
        std::cout << "pipeLine Destroyed.\n";
    }
   
    if (pipeLineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, pipeLineLayout, nullptr);
        std::cout << "pipeLine layout Destroyed.\n";
    }
    
    if (descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        std::cout << "Descriptor Pool Destroyed.\n";
    }
   
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        std::cout << "Descriptor Set layout Destroyed.\n";
    }
    
}

void ComputeShaders::recordDispatch(VkCommandBuffer cmdBuffer, uint32_t groupX, uint32_t groupY, uint32_t groupZ)
{
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeLine);
    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeLineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDispatch(cmdBuffer, groupX, groupY, groupZ);
}