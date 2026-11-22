#include "GraphicsPipeline.h"
#include "Vertex.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

std::vector<char> GraphicsPipeline::readFileVertex(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open())
        throw std::runtime_error("Failed to open file");

    size_t fileSize = static_cast<size_t>(file.tellg());

    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}


GraphicsPipeline::GraphicsPipeline(VkDevice device, VkFormat colorFormat, VkExtent2D extent,
    const std::vector<VkVertexInputBindingDescription>& bindings,
    const std::vector<VkVertexInputAttributeDescription>& attributes,
    const std::string& vertFile,
    const std::string& fragFile,
    VkFormat depthFormat,
    VkDescriptorSetLayout cameraDescriptorSetLayout,
    const GraphicsPipelineConfig& config
) : device(device)
{
    //==========================================================
    // Depth Stencil State
    //==========================================================

        VkPipelineDepthStencilStateCreateInfo depthStencil{};

        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

        depthStencil.depthTestEnable = config.depthWriteEnable; // This tells Vulkan to perform a depth comparison for fragments.
        depthStencil.depthWriteEnable = config.topology; // When a fragment passes the depth test, its depth value can be written into the depth image.
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // If the new fragment's depth is smaller than the depth already stored in the image, the fragment passes.

        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

    //==========================================================
    // Add the vertex file spv
    //==========================================================

    auto vertCode = readFileVertex(vertFile);
    auto fragCode = readFileVertex(fragFile);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = vertCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());

    VkShaderModule vertShaderModule;

    VkResult result = vkCreateShaderModule(
        device,
        &createInfo,
        nullptr,
        &vertShaderModule
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31mFailed to create shader module (vertex)\n");
    }
    std::cout << "\033[32m shader module (vertex) created succefully \033[0m\n";

    VkShaderModuleCreateInfo createInfo2{};
    createInfo2.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo2.codeSize = fragCode.size();
    createInfo2.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());

    VkShaderModule fragShaderModule;

    result = vkCreateShaderModule(
        device,
        &createInfo2,
        nullptr,
        &fragShaderModule
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31mFailed to create shader module (fragment)\n");
    }
    std::cout << "\033[32m shader module (fragment) created succefully\n";

    VkPipelineShaderStageCreateInfo vertexStage{};

    vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexStage.module = vertShaderModule;
    vertexStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStage{};

    fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStage.module = fragShaderModule;
    fragmentStage.pName = "main";

    //==========================================================
    // Vertex Input
    //==========================================================

    // Binding descriptions
    VkPipelineVertexInputStateCreateInfo vertexInput{};

    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
    vertexInput.pVertexBindingDescriptions = bindings.empty() ? nullptr : bindings.data();

    // Attribute descriptions
    vertexInput.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
    vertexInput.pVertexAttributeDescriptions = attributes.empty() ? nullptr : attributes.data();

    //==========================================================
    // create input assimbly to tell how we want read the vertix input
    //==========================================================

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};

    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // every 3 point represent a triangle, no reuse for the same points
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    //==========================================================
    // Viewport
    // this tells vulkan how the points (like 0.5,0.5) transfare to pixel coordinates
    //==========================================================
    VkViewport viewport{};

    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    //==========================================================
    // Scissor
    // this tells vulkan Which rectangular region of the framebuffer is allowed to receive fragments?
    //==========================================================
    VkRect2D scissor{};

    scissor.offset = { 0, 0 };
    scissor.extent = extent;

    //==========================================================
    // Viewport State
    //==========================================================

    VkPipelineViewportStateCreateInfo viewportState{};

    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;

    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;

    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    //==========================================================
    // rasterization, we answer Which locations on the screen are inside this triangle?
    //==========================================================

    VkPipelineRasterizationStateCreateInfo rasterizer{};

    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE; // Do not discard the geometry before rasterization.

    rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Fill the inside of the triangle.

    rasterizer.lineWidth = 1.0f;

    rasterizer.cullMode = VK_CULL_MODE_NONE; // discard back-facing triangles.
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    rasterizer.depthBiasEnable = VK_FALSE;

    //==========================================================
    // Multisampling
    // here you can choose to make the edge lines more smooth,
    // usually we check if the pixel center dot is inside the 3D object and if yes we fill the pixel with color,
    // Multisampling make 4 dots in every pixel and checks how many dots are inside the 3D model, 
    // then it blind the color of the pixel and the background color according to  how many dots are inside the model
    // and this tricks the eyes into seeing a smooth, clean line.
    // so a sample is the dot inside the pixel
    //==========================================================

    VkPipelineMultisampleStateCreateInfo multisampling{};

    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // Use one sample per pixel.

    //==========================================================
    // blending,
    // How should the color be combined with the existing framebuffer color?
    //==========================================================

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};

    colorBlendAttachment.blendEnable = config.blendingEnable; // Don't blend with the existing color.
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT |
        VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT |
        VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};

    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;

    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    //==========================================================
    // Pipeline Layout
    //==========================================================

    VkPushConstantRange pushConstantRange{};

    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};

    pipelineLayoutInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    // We now have one descriptor set layout.
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &cameraDescriptorSetLayout;

    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    result = vkCreatePipelineLayout(
        device,
        &pipelineLayoutInfo,
        nullptr,
        &pipelineLayout
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31mFailed to create pipeline layout.\n");
    }

    //==========================================================
    // creating the graphics pipeline
    //==========================================================

    VkPipelineRenderingCreateInfo pipelineRenderingInfo{};

    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    pipelineRenderingInfo.pColorAttachmentFormats = &colorFormat;
    pipelineRenderingInfo.depthAttachmentFormat = depthFormat;

    VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};

    graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineInfo.pNext = &pipelineRenderingInfo;

    VkPipelineShaderStageCreateInfo shaderStages[] =
    {
        vertexStage,
        fragmentStage
    };

    graphicsPipelineInfo.stageCount = 2;
    graphicsPipelineInfo.pStages = shaderStages;

    graphicsPipelineInfo.pVertexInputState = &vertexInput;
    graphicsPipelineInfo.pInputAssemblyState = &inputAssembly;
    graphicsPipelineInfo.pViewportState = &viewportState;
    graphicsPipelineInfo.pRasterizationState = &rasterizer;
    graphicsPipelineInfo.pMultisampleState = &multisampling;
    graphicsPipelineInfo.pColorBlendState = &colorBlending;
    graphicsPipelineInfo.pDepthStencilState = &depthStencil;

    graphicsPipelineInfo.layout = pipelineLayout;
    graphicsPipelineInfo.renderPass = VK_NULL_HANDLE;
    graphicsPipelineInfo.subpass = 0;

    //we're not deriving this pipeline from another existing pipeline so we set
    graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    graphicsPipelineInfo.basePipelineIndex = -1;

    result = vkCreateGraphicsPipelines(
        device,
        VK_NULL_HANDLE,
        1,
        &graphicsPipelineInfo,
        nullptr,
        &pipeline
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("\033[31m Failed to create graphics pipeline\n");
    }

    std::cout << "\033[32mgraphics pipeline created succefully\033[0m\n";

    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);

}

GraphicsPipeline::~GraphicsPipeline()
{
    destroy();
}

void GraphicsPipeline::bind(VkCommandBuffer commandBuffer) const
{
    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        pipeline
    );
}

void GraphicsPipeline::destroy()
{
    if (pipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(device, pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
    }

    if (pipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pipelineLayout = VK_NULL_HANDLE;
    }
}