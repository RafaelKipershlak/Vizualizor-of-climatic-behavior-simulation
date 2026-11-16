#include "VulkanSetup.h"
#include <iostream>
#include <vector>

VulkanSetup::VulkanSetup(uint32_t extensionCount, const char** extensions)
{
	
	if (extensionCount == 0 || extensions == nullptr)
	{
		std::cout << "No instance extensions requested.\n";
	}

	// =========================
	// 1. creating an instance
	// =========================

	std::vector<const char*> requiredExtensions;

	
	if (extensions != nullptr && extensionCount > 0)
    {
        requiredExtensions.assign(extensions, extensions + extensionCount);
    }

	#ifdef __APPLE__
	requiredExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
	requiredExtensions.push_back("VK_KHR_get_physical_device_properties2");
#endif



	// Specify Vulkan 1.3 so Dynamic Rendering and Synchronization2 work on Mac & Windows
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "4D Climate Visualizer";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "ClimateVulkanEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

	VkInstanceCreateInfo instanceInfo{};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	instanceInfo.ppEnabledExtensionNames = requiredExtensions.data();
	instanceInfo.pApplicationInfo = &appInfo;

	#ifdef __APPLE__
	instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

	VkResult result = vkCreateInstance(&instanceInfo, nullptr, &instance);

	if (result != VK_SUCCESS) {
		std::cout << "Falid to create instance\n";
		instance = VK_NULL_HANDLE; // Guard against garbage handle state
		return;
	}

	std::cout << "Vulkan instance created successfully!\n";

	// =========================
	// 2. select the GPU
	// =========================
	result = vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (result != VK_SUCCESS || deviceCount == 0)
	{
		std::cout << "No Vulkan-compatible GPU found!\n";
		return;
	}

	std::cout << "Number of GPUs found: " << deviceCount << "\n\n";

	std::vector<VkPhysicalDevice> devices(deviceCount);

	result = vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
	if (result != VK_SUCCESS)
	{
		std::cout << "Fialed to put devices in a vector!\n";
		return;
	}

	physicalGPU = devices[0];
	VkPhysicalDeviceProperties physicalGPUprop{};

	vkGetPhysicalDeviceProperties(physicalGPU, &physicalGPUprop);

	std::cout << "GPU: " << physicalGPUprop.deviceName << "\n";

	// =====================================================
	// 3. Find Queue Family
	// =====================================================

	vkGetPhysicalDeviceQueueFamilyProperties(physicalGPU, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalGPU, &queueFamilyCount, queueFamilies.data());

	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		std::cout << "\nQueue Family " << i << "\n";

		if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			std::cout << "  Graphics: YES\n";
			if (graphicsFamily == UINT32_MAX)
			{
				graphicsFamily = i; // Store the first valid graphics queue index
			}
		}
		else
		{
			std::cout << "  Graphics: NO\n";
		}

		if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
		{
			std::cout << "  Compute: YES\n";
		}
		else
		{
			std::cout << "  Compute: NO\n";
		}

		if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
		{
			std::cout << "  Transfer: YES\n";
		}
		else
		{
			std::cout << "  Transfer: NO\n";
		}

		std::cout << "  Queue count: "
			<< queueFamilies[i].queueCount
			<< "\n";
	}

	if (graphicsFamily == UINT32_MAX)
	{
		std::cout << "Failed to find a suitable graphics queue family!\n";
		return;
	}

	std::cout << "\n";

	// =====================================================
	// 4. Create the logical device and queue.
	// =====================================================

	//Create Queue
	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo queueInfo{};
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueFamilyIndex = graphicsFamily;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = &queuePriority;

	//Create Logical Device
	
	// Enable BOTH Synchronization2 (for Sync::bufferBarrier) AND Dynamic Rendering
    VkPhysicalDeviceSynchronization2Features sync2Features{};
    sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    sync2Features.synchronization2 = VK_TRUE;

	// for enabling dynamic rendering
	VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
	dynamicRenderingFeatures.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;

	dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
	dynamicRenderingFeatures.pNext = &sync2Features;

	const char* deviceExtensions[] =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME 
				#ifdef __APPLE__
		, "VK_KHR_portability_subset"
		#endif
	};

	VkDeviceCreateInfo deviceInfo{};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
	deviceInfo.ppEnabledExtensionNames = deviceExtensions;
	deviceInfo.pNext = &dynamicRenderingFeatures; // enable dynamic rendering

	result = vkCreateDevice(physicalGPU, &deviceInfo, nullptr, &device);

	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create logical device!\n";
		device = VK_NULL_HANDLE; // Guard against garbage handle state
		return;
	}

	std::cout << "Logical device created successfully!\n";

	// Get Graphics Queue
	vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = graphicsFamily;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	result = vkCreateCommandPool(
		device,
		&poolInfo,
		nullptr,
		&commandPool);

	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create command pool!\n";
		commandPool = VK_NULL_HANDLE; // Guard against garbage handle state
		return ;
	}

	std::cout << "Command pool created successfully!\n";
}

bool VulkanSetup::createCommandBuffers(uint32_t count) {
	
	if (commandPool == VK_NULL_HANDLE) {
		std::cout << "Cannot allocate command buffers: Command pool is null!\n";
		return false;
	}

	// Clean up old command buffers if re-allocating
	if (!commandBuffers.empty()) {
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		commandBuffers.clear();
	}

	// =====================================================
	// 5. Creating a command buffers
	// =====================================================
	commandBuffers.resize(count);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // A primary command buffer can be submitted directly to a queue
	allocInfo.commandBufferCount = count;

	VkResult result = vkAllocateCommandBuffers(
		device,
		&allocInfo,
		commandBuffers.data()
	);

	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to allocate command buffer!\n";
		return false;
	}

	std::cout << "Command buffer allocated successfully!\n";
	return true;
}

bool VulkanSetup::StartRecording(uint32_t bufferIndex) {
	// Check if command buffer was allocated successfully
	if (bufferIndex >= commandBuffers.size()) {
		std::cout << "Invalid command buffer index: " << bufferIndex << "\n";
		return false;
	}

	// Reset buffer before re-recording
	vkResetCommandBuffer(commandBuffers[bufferIndex], 0);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	// Flags control behavior:
	// VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT means the buffer will be re-recorded after execution
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkResult result = vkBeginCommandBuffer(commandBuffers[bufferIndex], &beginInfo);

	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to begin recording command buffer " << bufferIndex << "!\n";
		return false;
	}

	std::cout << "Command buffer " << bufferIndex << " recording started!\n";
	return true;
}

bool VulkanSetup::StopRecording(uint32_t bufferIndex) {

	if (bufferIndex >= commandBuffers.size()) {
		std::cout << "Invalid command buffer index: " << bufferIndex << "\n";
		return false;
	}

	//finished recording the buffer
	VkResult result = vkEndCommandBuffer(commandBuffers[bufferIndex]);
	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to end command buffer " << bufferIndex << "!\n";
		return false;
	}

	std::cout << "Command buffer " << bufferIndex << " recording finished!\n";
	return true;
}

void VulkanSetup::submitCommandBuffer(
	uint32_t bufferIndex,
	VkSemaphore waitSemaphore,
	VkSemaphore signalSemaphore,
	VkFence fence)
{
	if (bufferIndex >= commandBuffers.size()) {
		std::cout << "Invalid command buffer index: " << bufferIndex << "\n";
		return;
	}

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	// Wait semaphore (only if provided)
	if (waitSemaphore != VK_NULL_HANDLE)
	{
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
		submitInfo.pWaitDstStageMask = waitStages;
	}

	// Command buffer
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffers[bufferIndex];

	// Signal semaphore (only if provided)
	if (signalSemaphore != VK_NULL_HANDLE)
	{
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore;
	}

	VkResult result = vkQueueSubmit(
		graphicsQueue,
		1,
		&submitInfo,
		fence
	);
}

VulkanSetup::~VulkanSetup() {
	std::cout << "\n";

	
	// ONLY destroy command pool if both device and commandPool exist!
	if (device != VK_NULL_HANDLE && commandPool != VK_NULL_HANDLE) {
		vkDestroyCommandPool(device, commandPool, nullptr);// destroying the command pool automatically frees the command buffers allocated from it.
		std::cout << "Command Pool destroyed.\n";
	}

	if (device != VK_NULL_HANDLE) {
		vkDestroyDevice(device, nullptr);
		std::cout << "Logical device destroyed.\n";
	}

	if (instance != VK_NULL_HANDLE) {
		vkDestroyInstance(instance, nullptr);
		std::cout << "Vulkan instance destroyed.\n";
	}
}

