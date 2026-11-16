#pragma once
#include <iostream>
#include <vulkan/vulkan.h>
#include "VulkanSetup.h"
#include "VulkanBuffer.h"
#include <vector>
#include <chrono>
#include <fstream>
#include <stdexcept>

class ComputeShaders
{
	private:
		VkDevice device{ VK_NULL_HANDLE };
		VkPipeline pipeLine{ VK_NULL_HANDLE };
		VkPipelineLayout pipeLineLayout{ VK_NULL_HANDLE };
		VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
		VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
		VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };

	public:
		ComputeShaders(
			VkDevice device,
			const std::vector<const VulkanBuffer*>& bufferPtrs,
			const std::string& spvFileName
		);

		~ComputeShaders();

		// Prevent copying to avoid double-freeing Vulkan handles
		ComputeShaders(const ComputeShaders&) = delete;
		ComputeShaders& operator=(const ComputeShaders&) = delete;

		VkPipeline getPipeline() const { return pipeLine; }
		VkPipelineLayout getPipelineLayout() const { return pipeLineLayout; }
		VkDescriptorSet getDescriptorSet() const { return descriptorSet; }

		void recordDispatch(VkCommandBuffer cmdBuffer, uint32_t groupX, uint32_t groupY = 1, uint32_t groupZ = 1);

};

