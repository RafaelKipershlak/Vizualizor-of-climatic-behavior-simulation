#include "drawingIn3D.h"
#include <iostream>
#include <vulkan/vulkan.h>
#include "VulkanSetup.h"
#include "VulkanBuffer.h"
#include <vector>
#include <chrono>
#include <fstream>
#include <stdexcept>
#include "ComputeShaders.h"
#include "GraphicsPipeline.h"
#include "Vertex.h"
#include "Synchronization.h"
#include "VulkanSurface.h"
#include "Swapchain.h"
#include "FrameSync.h"
#include <GLFW/glfw3.h>
#include "DepthResources.h"
#include "Camera.h"
#include "Texture.h"
#include <cmath>

std::vector<EarthVertex> generateSphereVertices(
    uint32_t slices,
    uint32_t stacks,
    float radius)
{
    std::vector<EarthVertex> vertices;

    const float PI = 3.14159265359f;

    for (uint32_t i = 0; i <= stacks; i++)
    {
        float v = static_cast<float>(i) / static_cast<float>(stacks);
        float phi = PI * v; // Latitude angle

        for (uint32_t j = 0; j <= slices; j++)
        {
            float u = static_cast<float>(j) / static_cast<float>(slices);
            float theta = 2.0f * PI * u; // Longitude angle

            float nx = std::sin(phi) * std::cos(theta);
            float ny = std::cos(phi);
            float nz = std::sin(phi) * std::sin(theta);

            EarthVertex vertex{};

            vertex.pos[0] = radius * nx;
            vertex.pos[1] = radius * ny;
            vertex.pos[2] = radius * nz;

            vertex.normal[0] = nx;
            vertex.normal[1] = ny;
            vertex.normal[2] = nz;

            vertex.uv[0] = 1.0f -u;
            vertex.uv[1] = v;

            vertices.push_back(vertex);
        }
    }

    return vertices;
}

std::vector<uint32_t> generateSphereIndices(
    uint32_t slices,
    uint32_t stacks)
{
    std::vector<uint32_t> indices;

    for (uint32_t i = 0; i < stacks; i++)
    {
        for (uint32_t j = 0; j < slices; j++)
        {
            uint32_t topLeft = i * (slices + 1) + j;
            uint32_t bottomLeft = (i + 1) * (slices + 1) + j;

            uint32_t topRight = topLeft + 1;
            uint32_t bottomRight = bottomLeft + 1;

            // First triangle
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Second triangle
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    return indices;
}

static int MAX_FRAMES_IN_FLIGHT = 2;

void draw(VulkanSetup* vkSetup, GLFWwindow* window)
{
    VkDevice device = vkSetup->getDevice();
    VkPhysicalDevice physicalDevice = vkSetup->getPhysicalDevice();

    // 1. Create surface
    VulkanSurface surface(vkSetup->getInstance(), window, physicalDevice, vkSetup->getGraphicsFamily());

    // 2. Create swapchain
    Swapchain swapchain(surface, device);
    const uint32_t MAX_FRAMES_IN_FLIGHT = 1;

    FrameSync frameSync(
        device,
        MAX_FRAMES_IN_FLIGHT,
        swapchain.getSwapchainImageCount()
    );

    // 2.1 Get swapchain dimensions
    VkExtent2D extend = surface.getExtent();

    // 3. Create depth resources
    DepthResources depthResourses(physicalDevice, device, extend);

    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings{};

    // Binding 0: Camera Uniform Buffer (Vertex Shader)
    layoutBindings[0].binding            = 0;
    layoutBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layoutBindings[0].descriptorCount    = 1;
    layoutBindings[0].stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    layoutBindings[0].pImmutableSamplers = nullptr;

    // Binding 1: Earth Texture Sampler (Fragment Shader)
    layoutBindings[1].binding            = 1;
    layoutBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBindings[1].descriptorCount    = 1;
    layoutBindings[1].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBindings[1].pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo cameraLayoutInfo{};
    cameraLayoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    cameraLayoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    cameraLayoutInfo.pBindings    = layoutBindings.data();

    VkDescriptorSetLayout cameraDescriptorSetLayout = VK_NULL_HANDLE;

    VkResult result = vkCreateDescriptorSetLayout(
        device,
        &cameraLayoutInfo,
        nullptr,
        &cameraDescriptorSetLayout
    );

    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create camera descriptor set layout!");
    }

    // 4. Create graphics pipeline

    //Vertex binding description, this Tells Vulkan how our vertices are arranged in memory.
    std::vector<VkVertexInputBindingDescription> bindings = {
    EarthVertex::getBindingDescription()
};

    //Vertex attribute description, Tells Vulkan which part of each vertex represents the position.
    auto earthAttrs = EarthVertex::getAttributeDescriptions();
    std::vector<VkVertexInputAttributeDescription> attributes(earthAttrs.begin(), earthAttrs.end());



    //std::vector<VkVertexInputAttributeDescription> attributes = { attribute };

    GraphicsPipelineConfig meshConfig{};

    meshConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // means every three vertices form a triangle.
    meshConfig.depthWriteEnable = VK_TRUE; // means depth values are written.
    meshConfig.blendingEnable = VK_FALSE; // means blending is disabled.

    GraphicsPipeline pipeline(
        device,
        surface.getFormat().format,
        extend,
        bindings,
        attributes,
        "shaders/sphere.vert.spv",
        "shaders/sphere.frag.spv",
        depthResourses.getFormat(),
        cameraDescriptorSetLayout,
        meshConfig
    );
    
    // create a renering depth attachment
    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    depthAttachment.imageView = depthResourses.getImageView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };   // Clear depth to 1.0

    // creating buffers

    /*
    std::vector<VertexCube> vertices =
    {
        {-1.0f, -1.0f, -1.0f}, // 0
        { 1.0f, -1.0f, -1.0f}, // 1
        { 1.0f,  1.0f, -1.0f}, // 2
        {-1.0f,  1.0f, -1.0f}, // 3

        {-1.0f, -1.0f,  1.0f}, // 4
        { 1.0f, -1.0f,  1.0f}, // 5
        { 1.0f,  1.0f,  1.0f}, // 6
        {-1.0f,  1.0f,  1.0f}  // 7
    };

    std::vector<uint32_t> indices =
    {
        // Back face
        0, 2, 1,
        2, 0, 3,

        // Front face
        4, 5, 6,
        6, 7, 4,

        // Left face
        0, 4, 7,
        7, 3, 0,

        // Right face
        1, 2, 6,
        6, 5, 1,

        // Bottom face
        0, 1, 5,
        5, 4, 0,

        // Top face
        3, 7, 6,
        6, 2, 3
    };
    */

    const uint32_t slices = 64;
    const uint32_t stacks = 32;
    const float radius = 1.0f;

    std::vector<EarthVertex> vertices = generateSphereVertices(slices, stacks, radius);

    std::vector<uint32_t> indices = generateSphereIndices(slices, stacks);

    Camera::Mat4 model = { {
        {{1.0f, 0.0f, 0.0f, 0.0f}},
        {{0.0f, 1.0f, 0.0f, 0.0f}},
        {{0.0f, 0.0f, 1.0f, 0.0f}},
        {{0.0f, 0.0f, 0.0f, 1.0f}}
    } };

    VkDeviceSize vBufferSize = sizeof(EarthVertex) * vertices.size();

    VulkanBuffer vStagingBuffer(
        device,
        physicalDevice,
        vBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::CpuToGpu
    );

    vStagingBuffer.copyVector(vertices);

    VkDeviceSize iBufferSize = sizeof(uint32_t) * indices.size();

    VulkanBuffer iStagingBuffer(
        device,
        physicalDevice,
        iBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        MemoryType::CpuToGpu
    );

    iStagingBuffer.copyVector(indices);

    // GPU only buffers
    VulkanBuffer verticesBuffer(
        device,
        physicalDevice,
        vBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        MemoryType::GpuOnly
    );

    VulkanBuffer indicesBuffer(
        device,
        physicalDevice,
        iBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        MemoryType::GpuOnly
    );

    VulkanBuffer cameraBuffer(
        device,
        physicalDevice,
        sizeof(CameraUniform),
        BufferType::Uniform,
        MemoryType::CpuToGpu
    );

    // Load Earth texture into VkImage
    Texture earthTexture(
        physicalDevice,
        device,
        vkSetup->getCommandPool(),
        vkSetup->getGraphicsQueue(),
        "assets/textures/earth.jpg"
    );

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
   
    poolSizes[0].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = 1;

    VkDescriptorPool cameraDescriptorPool = VK_NULL_HANDLE;

    result = vkCreateDescriptorPool(
        device,
        &poolInfo,
        nullptr,
        &cameraDescriptorPool
    );

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to create camera descriptor pool!");

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = cameraDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &cameraDescriptorSetLayout;

    VkDescriptorSet cameraDescriptorSet = VK_NULL_HANDLE;

    result = vkAllocateDescriptorSets(
        device,
        &allocInfo,
        &cameraDescriptorSet
    );

    if (result != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate camera descriptor set!");

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = cameraBuffer.getBuffer();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(CameraUniform);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = earthTexture.getImageView();
    imageInfo.sampler = earthTexture.getSampler();

    
    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = cameraDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].dstArrayElement = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &bufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = cameraDescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].dstArrayElement = 0;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(
        device,
        static_cast<uint32_t>(descriptorWrites.size()),
        descriptorWrites.data(),
        0,
        nullptr
    );

    for (size_t i = 0; i < vkSetup->getCommandBuffersCount(); i++)
    {
        if (vkSetup->StartRecording(i))
        {
            VkBufferCopy reg{};
            reg.size = vBufferSize;
            reg.srcOffset = 0;
            reg.dstOffset = 0;

            vkCmdCopyBuffer(
                vkSetup->getCommandBuffer(i),
                vStagingBuffer.getBuffer(),
                verticesBuffer.getBuffer(),
                1,
                &reg
            );

            Sync::bufferBarrier(
                vkSetup->getCommandBuffer(i),
                verticesBuffer.getBuffer(),
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
                VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT,
                0,
                vBufferSize
            );

            VkBufferCopy reg2{};
            reg2.size = iBufferSize;
            reg2.srcOffset = 0;
            reg2.dstOffset = 0;

            vkCmdCopyBuffer(
                vkSetup->getCommandBuffer(i),
                iStagingBuffer.getBuffer(),
                indicesBuffer.getBuffer(),
                1,
                &reg2
            );


            Sync::bufferBarrier(
                vkSetup->getCommandBuffer(i),
                indicesBuffer.getBuffer(),
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT,
                VK_ACCESS_2_INDEX_READ_BIT,
                0,
                iBufferSize
            );

            vkSetup->StopRecording(i);
            vkSetup->submitCommandBuffer(i);
        }
    }

    vkQueueWaitIdle(vkSetup->getGraphicsQueue());

    int currentFrame = 0;
    uint32_t imageIndex = 0;
    bool depthInitialized = false;

    Camera camera(surface.getExtent().width, surface.getExtent().height);

    //keep the window open
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        //wait for the current frame previos rendering and presentation to finish
        VkFence currentFence = frameSync.getInFlightFence(currentFrame);
        vkWaitForFences(
            device,
            1,
            &currentFence,
            VK_TRUE,
            UINT64_MAX
        );

        // get the next avilable image in the swapchain
        
        VkResult result = vkAcquireNextImageKHR(
            device,
            swapchain.getSwapchain(),
            UINT64_MAX,
            frameSync.getImageAvailable(currentFrame),
            VK_NULL_HANDLE,
            &imageIndex
        );

        if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
        {
            throw std::runtime_error("Failed to acquire swapchain image!");
        }

        // kAcquireNextImageKHR only checks whether the windowing system's presentation engine is done displaying the image,
        // and has released it back to the swapchain.
        // It does not check whether your own GPU has finished executing the rendering commands you previously submitted to that image.
        // SO WE CHECK if the GPU has finished rendring to that image
        VkFence fence = frameSync.getImagesInFlight(imageIndex);
        if (fence != VK_NULL_HANDLE)
        {
            vkWaitForFences(
                device,
                1,
                &fence,
                VK_TRUE,
                UINT64_MAX
            );
        }

        // link the current image to the current frame
        frameSync.setImagesInFlight(imageIndex, currentFrame);

        vkResetFences(device, 1, &currentFence);
        //==========================================================
        // building image memory barrier
        //==========================================================

        const auto& swapchainImages = swapchain.getSwapchainImages();
        auto& swapchainImageLayouts = swapchain.getSwapchainImageLayouts();
        const auto& swapchainImageViews = swapchain.getSwapchainImageViews();

        // create a renering color attachment
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

        colorAttachment.imageView = swapchainImageViews[imageIndex];
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store the rendered image so it can be presented

        colorAttachment.clearValue.color = { { 0.1f, 0.1f, 0.1f, 1.0f } };

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;

        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.renderArea.extent = extend;

        renderingInfo.layerCount = 1;

        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        renderingInfo.pDepthAttachment = &depthAttachment;

        //angleX += 0.01f;
        //angleY += 0.01f;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
        {
            double mouseX, mouseY;

            glfwGetCursorPos(window, &mouseX, &mouseY);

            camera.updateCameraYaw(mouseX, mouseY);
        }
        else
        {
            camera.setFirstMouse(true);
        }
        
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            camera.updateHorizontalPosition(0.1f);
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            camera.updateHorizontalPosition(-0.1f);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            camera.updateVerticalPosition(0.1f); // move forward
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            camera.updateVerticalPosition(-0.1f); // move backwards
        }

       
        // Gets the latest projection and view matrices from your camera. 
        // And Copies both matrices into the uniform buffer. we do that after input processing b
        // because we want the matrices to reflect the camera's latest position and orientation for this frame.
        CameraUniform cameraUniform{
            camera.getProjectionMatrix(),
            camera.getViewMatrix()
        };

        cameraBuffer.copyTo(&cameraUniform, sizeof(CameraUniform));

        // render the image
        if (vkSetup->StartRecording(currentFrame))
        {
            VkCommandBuffer commandBuffer = vkSetup->getCommandBuffer(currentFrame);

            // start drawing / rendering

            Sync::imageMemoryBarrier(
                commandBuffer,
                swapchainImages[imageIndex],

                swapchainImageLayouts[imageIndex],
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,

                VK_PIPELINE_STAGE_2_NONE,
                VK_ACCESS_2_NONE,

                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
            );

            swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // transfare the depth image layout and use
            if (!depthInitialized)
            {
                depthResourses.transitionLayout(commandBuffer);
                depthInitialized = true;
            }

            // Begin rendering
            vkCmdBeginRendering(commandBuffer, &renderingInfo);

            // Tell Vulkan which graphics pipeline to use for subsequent draw commands.
            pipeline.bind(commandBuffer);

            // our shaders expext layout(set = 0, binding = 0) uniform CameraData
            vkCmdBindDescriptorSets(
                commandBuffer,
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                pipeline.getPipelineLayout(),
                0,                          // First descriptor set
                1,                          // Number of descriptor sets
                &cameraDescriptorSet,
                0,                          // Dynamic offset count
                nullptr
            );

            //Tell Vulkan where to retrieve vertex data.
            VkBuffer vertexBufferHandle = verticesBuffer.getBuffer();
            VkDeviceSize vertexOffset = 0;
            vkCmdBindVertexBuffers(
                commandBuffer,
                0,
                1,
                &vertexBufferHandle,
                &vertexOffset // pointer to the buffer offset
            );

            // tell Vulkan that our indices are 32-bit unsigned integers.
            vkCmdBindIndexBuffer(
                commandBuffer,
                indicesBuffer.getBuffer(),
                0,
                VK_INDEX_TYPE_UINT32
            );

            camera.updateCameraYaw();
            camera.updateCameraPitch();

            // 2. Initialize the struct using proper C++ curly braces {}
            PushConstants pushConstants = {
                model
            };

            vkCmdPushConstants(
                commandBuffer,
                pipeline.getPipelineLayout(),
                VK_SHADER_STAGE_VERTEX_BIT,
                0,
                sizeof(PushConstants),
                &pushConstants
            );

            // Draw
            vkCmdDrawIndexed(
                commandBuffer,
                static_cast<uint32_t>(indices.size()), // Number of indices to read
                1,                                     // Draw one cube
                0,                                     // Start at index 0
                0,                                     // No vertex offset
                0                                      // Start at instance 0
            );

            // End rendering
            vkCmdEndRendering(commandBuffer);

            // transfare image to present mode
            Sync::imageMemoryBarrier(
                commandBuffer,
                swapchainImages[imageIndex],

                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,

                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,

                VK_PIPELINE_STAGE_2_NONE,
                VK_ACCESS_2_NONE
            );

            swapchainImageLayouts[imageIndex] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            vkSetup->StopRecording(currentFrame);
            vkSetup->submitCommandBuffer(
                currentFrame,
                frameSync.getImageAvailable(currentFrame),
                frameSync.getRenderFinished(currentFrame),
                currentFence
            );
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1; // We have one semaphore that presentation must wait for.
        
        VkSemaphore currentRenderFinishedSemaphore = frameSync.getRenderFinished(currentFrame);
        presentInfo.pWaitSemaphores = &currentRenderFinishedSemaphore; // Presentation waits until rendering has finished.

        presentInfo.swapchainCount = 1; // We are presenting to one swapchain.

        VkSwapchainKHR swapchainHandle = swapchain.getSwapchain();
        presentInfo.pSwapchains = &swapchainHandle; // The swapchain containing the image.

        presentInfo.pImageIndices = &imageIndex;

        VkResult presentResult = vkQueuePresentKHR(vkSetup->getGraphicsQueue(), &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR) {
            // Handle resize
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }
    // Ensure the GPU is completely idle before local resources start destroying
    vkDeviceWaitIdle(device);

    // 1. Destroy graphics pipeline and pipeline layout
    pipeline.destroy();

    // 2. Destroy descriptor pool (also frees cameraDescriptorSet)
    if (cameraDescriptorPool != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(
            device,
            cameraDescriptorPool,
            nullptr
        );

        cameraDescriptorPool = VK_NULL_HANDLE;
    }

    // 3. Destroy descriptor set layout
    if (cameraDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(
            device,
            cameraDescriptorSetLayout,
            nullptr
        );

        cameraDescriptorSetLayout = VK_NULL_HANDLE;
    }
}

int drawCube()
{
    //initialize GLFW
    if (!glfwInit())
    {
        std::cout << "\033[31m GLFW initialization failed!\n";
        return -1;
    }

    //disable openGL
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    
    //create a window
    GLFWwindow* window = glfwCreateWindow(
        1280,
        720,
        "cube drawing",
        nullptr,
        nullptr
    );

    if (!window)
    {
        std::cout << "\033[31mWindow creation failed!\n";
        glfwTerminate();
        return -1;
    }

    std::cout << "\033[32m Window created successfully! \033[0m \n";

    //create extensions to connect to vulkan instanse 
    uint32_t extintionCount = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&extintionCount);

    if (!extensions)
    {
        std::cout << "\033[31mFailed to get GLFW Vulkan extensions!\n";

        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    //add a scope so the vulkan setup get destroyed before the GLFW window
    {
        //connect to the vulkan setup
        VulkanSetup vkSetup(extintionCount, extensions);
        vkSetup.createCommandBuffers();
        draw(&vkSetup, window);
    }
    
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

int main()
{
    drawCube();
    return 0;
}