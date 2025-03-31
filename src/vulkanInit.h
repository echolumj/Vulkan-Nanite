#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

namespace vk {

//index of queue family
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;//物理设备对应的队列族
	std::optional<uint32_t> presentFamily;
	std::optional<uint32_t> graphicAndComputeFamily;

	bool isComplete() {
		return graphicAndComputeFamily.has_value() && presentFamily.has_value();
	}
};

//basic info of swap chain creation
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};


class VulkanInit {
public:

	static void createInstance(VkInstance& instance, VkDebugUtilsMessengerEXT& debugMessenger, bool enableValidationLayers);
	static void createSurface(VkInstance& instance, GLFWwindow* window, VkSurfaceKHR& surface);
	static void pickPhysicalDevice(VkPhysicalDevice& pdev, VkInstance& instance, VkSurfaceKHR& surface, const std::vector<const char*>& extensions);

	static void createBuffer(VkPhysicalDevice &physicalDev, VkDevice &logicalDev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	static void copyBuffer(VkDevice& logicalDev, VkQueue& queue, VkCommandPool& cp, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	static void createImage(VkPhysicalDevice& physicalDev, VkDevice& logicalDev, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
		VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	static void createImageView(VkDevice& logicalDev, VkImage image, VkFormat format, VkImageAspectFlags aspectMask, VkImageView& imageView);
	static void transitionImageLayout(VkDevice& dev, VkQueue& queue, VkCommandPool& cp, VkImage& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	static void endSingleTimeCommands(VkDevice& device, VkQueue& queue, VkCommandBuffer& commandBuffer, VkCommandPool& cmdPool);
	static VkCommandBuffer beginSingleTimeCommands(VkDevice& device, VkCommandPool& cmdPool);

	static uint32_t findMemoryType(VkPhysicalDevice &physicalDev, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice &device, VkSurfaceKHR &surface);

	static VkShaderModule createShaderModule(VkDevice& device, const std::vector<char>& code);

	static bool CheckLayerSupport(const std::vector<const char*>& layers);
	static std::vector<const char*> getRequiredExtensions(bool enableValidationLayers);

	static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	static void DestoryDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	static bool CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& extensions);
	static vk::SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR& surface);
	static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR& surface, const std::vector<const char*>& extensions);
};

}