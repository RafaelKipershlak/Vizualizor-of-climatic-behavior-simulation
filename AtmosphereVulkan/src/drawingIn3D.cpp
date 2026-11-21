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

    // 4. Create graphics pipeline

    //Vertex binding description, this Tells Vulkan how our vertices are arranged in memory.
    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = sizeof(VertexCube);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::vector<VkVertexInputBindingDescription> bindings = { binding };

    //Vertex attribute description, Tells Vulkan which part of each vertex represents the position.
    VkVertexInputAttributeDescription attribute{};
    attribute.binding = 0;
    attribute.format = VK_FORMAT_R32G32B32_SFLOAT; //Three 32-bit floating-point values (X, Y, Z).
    attribute.location = 0; // Corresponds to shader input location 0.
    attribute.offset = 0; // Position starts at the beginning of our structure.

    std::vector<VkVertexInputAttributeDescription> attributes = { attribute };

    GraphicsPipeline pipeline(device, surface.getFormat().format, extend, bindings, attributes, "shaders/cube.vert.spv", "shaders/cube.frag.spv", depthResourses.getFormat());

    // create a renering depth attachment
    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;

    depthAttachment.imageView = depthResourses.getImageView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;

    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    depthAttachment.clearValue.depthStencil = { 1.0f, 0 };   // Clear depth to 1.0

    // creating buffers

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

    VkDeviceSize vBufferSize = sizeof(VertexCube) * vertices.size();

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

    float angleX = 0.0f;
    float angleY = 0.0f;

    float fovDegrees = 60.0f;
    float fovRadians = fovDegrees * (3.14159265358979323846f / 180.0f);
    float f = 1.0f / std::tan(fovRadians / 2.0f);
    float aspect = static_cast<float>(surface.getExtent().width) / static_cast<float>(surface.getExtent().height);
    float farPlane = 100.0f;
    float nearPlane = 0.1f;

    float cameraYawDegrees = 0;
    float cameraYaw = cameraYawDegrees * (3.14159265358979323846f / 180.0f);
    float c = std::cos(cameraYaw);
    float s = std::sin(cameraYaw);

    float cameraPitchDegrees = 0.0f; // 1. Add pitch tracking
    float pitch = cameraPitchDegrees * (3.14159265358979323846f / 180.0f);
    float cp = std::cos(pitch);
    float sp = std::sin(pitch);

    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    bool firstMouse = true;

    float mouseSensitivity = 0.1f;

    float cameraPosForward = -5.0f;
    float cameraPosRight = 0.0f;

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

            if (firstMouse)
            {
                lastMouseX = mouseX;
                lastMouseY = mouseY;
                firstMouse = false;
            }

            double deltaX = mouseX - lastMouseX;
            double deltaY = mouseY - lastMouseY;

            lastMouseX = mouseX;
            lastMouseY = mouseY;

            // Update camera angles
            cameraYawDegrees -= static_cast<float>(deltaX) * mouseSensitivity;
            cameraPitchDegrees += static_cast<float>(deltaY) * mouseSensitivity;
        }
        else
        {
            firstMouse = true;
        }
        
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            cameraPosRight += 0.1f;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            cameraPosRight -= 0.1f;
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
            cameraPosForward += 0.1f; // move forward
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
            cameraPosForward -= 0.1f; // move backwards
        }

        // Clamp pitch so the camera doesn't flip upside down past the poles
        //if (cameraPitchDegrees > 89.0f)  cameraPitchDegrees = 89.0f;
        //if (cameraPitchDegrees < -89.0f) cameraPitchDegrees = -89.0f;
        


        //cameraYawDegrees += 0.1f;

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

            cameraYaw = cameraYawDegrees * (3.14159265358979323846f / 180.0f);
            c = std::cos(cameraYaw);
            s = std::sin(cameraYaw);

            pitch = cameraPitchDegrees * (3.14159265358979323846f / 180.0f);
            cp = std::cos(pitch);
            sp = std::sin(pitch);

            // 2. Initialize the struct using proper C++ curly braces {}
            PushConstants pushConstants = {
                angleX,
                angleY,
                {0,0},
                {
                    { f / aspect,  0.0f,  0.0f,                                             0.0f },
                    { 0.0f,        -f,     0.0f,                                             0.0f },
                    { 0.0f,        0.0f,  farPlane / (nearPlane - farPlane),                -1.0f },
                    { 0.0f,        0.0f,  (farPlane * nearPlane) / (nearPlane - farPlane),  0.0f }
                },
                {
                    { c,        s*sp,   s*cp,               0.0f },
                    { 0.0f,     cp,     -sp,                0.0f },
                    { -s,       c*sp,   c*cp,               0.0f }, 
                    { cameraPosRight,     0.0f,   cameraPosForward,   1.0f }
                }
            };

            vkCmdPushConstants(
                commandBuffer,
                pipeline.getPipelineLayout(),  // We need a getter for the pipeline layout
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