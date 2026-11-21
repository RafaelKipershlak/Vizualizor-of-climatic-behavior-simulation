#include "VulkanBuffer.h"
#include <iostream>
#include <stdexcept>
#include <cstring>

static VkBufferUsageFlags getVulkanUsage(BufferType type) {
	switch (type) {
	case BufferType::Storage:    return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	case BufferType::Vertex:     return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	case BufferType::Index:      return VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	case BufferType::Uniform:    return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	case BufferType::StagingSrc: return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	case BufferType::StagingDst: return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	return 0;
}

static VkMemoryPropertyFlags getVulkanMemoryProperties(MemoryType memory) {
	switch (memory) {
	case MemoryType::CpuToGpu:
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	case MemoryType::GpuOnly:
		return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	case MemoryType::GpuToCpu:
		return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
	}
	return 0;
}

VulkanBuffer::VulkanBuffer(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	VkDeviceSize size,
	VkBufferUsageFlags usageFlags,
	MemoryType memoryLocation,
	VkSharingMode sharingMode,
	const std::vector<uint32_t>& queueFamilyIndices
) : device(device), size(size) {

	// =====================================================
	// Create a vulkan buffer
	// =====================================================
	VkBufferCreateInfo bufferInfo{};

	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usageFlags;
	bufferInfo.sharingMode = sharingMode;

	// Fix 2: Handle concurrent queue indices if provided
	if (sharingMode == VK_SHARING_MODE_CONCURRENT && !queueFamilyIndices.empty()) {
		bufferInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
		bufferInfo.pQueueFamilyIndices = queueFamilyIndices.data();
	}

	VkResult result = vkCreateBuffer(
		device,
		&bufferInfo,
		nullptr,
		&buffer);

	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to create VKbuffer!\n";
		buffer = VK_NULL_HANDLE;
		return;
	}
	std::cout << " VKBuffer created successfully!\n";

	// =====================================================
	// Allocate memory for the vulkan buffer
	// =====================================================

	VkMemoryRequirements memRequirements{};
	vkGetBufferMemoryRequirements(
		device,
		buffer,
		&memRequirements
	);

	VkMemoryAllocateInfo memoryAllocInfo{};
	memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocInfo.allocationSize = memRequirements.size;
	memoryAllocInfo.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, getVulkanMemoryProperties(memoryLocation));

	result = vkAllocateMemory(
		device,
		&memoryAllocInfo,
		nullptr,
		&memory
	);

	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to allocate Memory !\n";
		vkDestroyBuffer(device, buffer, nullptr); // Fix 3: Clean up buffer handle on failure
		buffer = VK_NULL_HANDLE;
		memory = VK_NULL_HANDLE;
		return;
	}

	std::cout << " Memory allocated successfully!\n";

	result = vkBindBufferMemory(
		device,
		buffer,
		memory,
		0 //memory offset
	);

	if (result != VK_SUCCESS)
	{
		std::cout << "Failed to bind Memory !\n";
		vkFreeMemory(device, memory, nullptr);
		vkDestroyBuffer(device, buffer, nullptr);
		buffer = VK_NULL_HANDLE;
		memory = VK_NULL_HANDLE;
		return ;
	}

	std::cout << " Memory binded successfully!\n";

	// Persistent host mapping for CPU-accessible buffers
    if (memoryLocation == MemoryType::CpuToGpu || memoryLocation == MemoryType::GpuToCpu)
    {
        result = vkMapMemory(device, memory, 0, size, 0, &mappedPtr);
        if (result != VK_SUCCESS)
        {
            std::cout << "Failed to persistently map buffer memory!\n";
            mappedPtr = nullptr;
        }
    }
}

// 2. Delegating Constructor (converts BufferType to VkBufferUsageFlags and delegates)
VulkanBuffer::VulkanBuffer(
	VkDevice device,
	VkPhysicalDevice physicalDevice,
	VkDeviceSize size,
	BufferType type,
	MemoryType memoryLocation,
	VkSharingMode sharingMode,
	const std::vector<uint32_t>& queueFamilyIndices
) : VulkanBuffer(
	device,
	physicalDevice,
	size,
	getVulkanUsage(type), // Converts VkBufferUsageFlags -> BufferType
	memoryLocation,
	sharingMode,
	queueFamilyIndices
)
{
	// Body is empty! Everything is executed by the delegated constructor above.
}



VulkanBuffer::~VulkanBuffer() {
	std::cout << "\n";
	if (mappedPtr != nullptr && memory != VK_NULL_HANDLE) {
        vkUnmapMemory(device, memory);
        mappedPtr = nullptr;
    }
	
	if (buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, buffer, nullptr);
		std::cout << "VkBuffer destroyed.\n";
	}
	
	if (memory != VK_NULL_HANDLE) {
		vkFreeMemory(device, memory, nullptr);
		std::cout << "Buffer memory freed.\n";
	}
}

uint32_t VulkanBuffer::findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("Failed to find suitable memory type!");
}


void VulkanBuffer::copyTo(const void* data, VkDeviceSize dataSize, VkDeviceSize offset) {
    if (mappedPtr == nullptr) {
        std::cout << "Cannot copy data: Buffer is not host-mapped (is it GpuOnly?)!\n";
        return;
    }

    if (offset + dataSize > size) {
        std::cout << "Cannot copy data: Target size exceeds buffer size!\n";
        return;
    }

    // Cast to char* for safe byte-level pointer arithmetic with offset
    std::memcpy(static_cast<char*>(mappedPtr) + offset, data, static_cast<size_t>(dataSize));
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
    : device(other.device),
      buffer(other.buffer),
      memory(other.memory),
      size(other.size),
      mappedPtr(other.mappedPtr)
{
    other.device = VK_NULL_HANDLE;
    other.buffer = VK_NULL_HANDLE;
    other.memory = VK_NULL_HANDLE;
    other.size = 0;
    other.mappedPtr = nullptr;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
{
    if (this != &other)
    {
        // 1. Clean up any existing resources owned by *this
        if (mappedPtr != nullptr && memory != VK_NULL_HANDLE) {
            vkUnmapMemory(device, memory);
        }
        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, memory, nullptr);
        }
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(device, buffer, nullptr);
        }

        // 2. Steal resources from other
        device = other.device;
        buffer = other.buffer;
        memory = other.memory;
        size = other.size;
        mappedPtr = other.mappedPtr;

        // 3. Leave other in a safe, empty state
        other.device = VK_NULL_HANDLE;
        other.buffer = VK_NULL_HANDLE;
        other.memory = VK_NULL_HANDLE;
        other.size = 0;
        other.mappedPtr = nullptr;
    }
    return *this;
}