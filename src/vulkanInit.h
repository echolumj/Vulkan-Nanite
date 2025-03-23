#pragma once
#include <vulkan/vulkan.h>
#include <optional>
#include <vector>

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

class VulkanInit {
public:
	static void createBuffer(VkPhysicalDevice &physicalDev, VkDevice &logicalDev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
	static void copyBuffer(VkDevice& logicalDev, VkQueue& queue, VkCommandPool& cp, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	static void endSingleTimeCommands(VkDevice& device, VkQueue& queue, VkCommandBuffer& commandBuffer, VkCommandPool& cmdPool);
	static VkCommandBuffer beginSingleTimeCommands(VkDevice& device, VkCommandPool& cmdPool);

	static uint32_t findMemoryType(VkPhysicalDevice &physicalDev, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	static QueueFamilyIndices VulkanInit::findQueueFamilies(VkPhysicalDevice &device, VkSurfaceKHR &surface);

	static VkShaderModule createShaderModule(VkDevice& device, const std::vector<char>& code);
};

}