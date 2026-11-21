#pragma once
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vector>

class VulkanSurface
{
	private:
		VkInstance instance = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		GLFWwindow* window = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkBool32 presentSupport = VK_FALSE;
		VkSurfaceCapabilitiesKHR capabilities{};
		VkSurfaceFormatKHR surfaceFormat{};
		VkPresentModeKHR presentMode{};
		VkExtent2D extent{};

		VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& presentModes);
		VkSurfaceFormatKHR chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
		const char* formatToString(VkFormat format);

	public:
		VulkanSurface(VkInstance instance, GLFWwindow* window, VkPhysicalDevice physicalDevice, uint32_t graphicsFamily);
		~VulkanSurface();

		VkSurfaceKHR getSurface() const { return surface; };
		const VkSurfaceCapabilitiesKHR& getCapabilities() const { return capabilities; };
		VkSurfaceFormatKHR getFormat() const { return surfaceFormat; };
		VkPresentModeKHR getPresentMode() const { return presentMode; };
		VkExtent2D getExtent() const { return extent; };
		bool supportsPresentation() const { return presentSupport; };

		void refresh();
};

