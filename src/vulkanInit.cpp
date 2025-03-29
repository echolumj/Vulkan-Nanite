#include "vulkanInit.h"
#include <iostream>
using namespace vk;

void VulkanInit::createBuffer(VkPhysicalDevice &physicalDev, VkDevice &logicalDev, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(logicalDev, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(logicalDev, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(physicalDev, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(logicalDev, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(logicalDev, buffer, bufferMemory, 0);
}

void VulkanInit::copyBuffer(VkDevice& logicalDev, VkQueue &queue, VkCommandPool &cp, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cp;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDev, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(logicalDev, cp, 1, &commandBuffer);
}

/******************************IMAGE***********************************/
void VulkanInit::createImage(VkPhysicalDevice& physicalDev, VkDevice& logicalDev, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
	VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	//Vk图像对象创建
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;      //Vulkan支持多种图像格式，但无论如何我们要在缓冲区中为纹素应用与像素一致的格式，否则拷贝操作会失败。(VK_FORMAT_R8G8B8A8_SRGB)
	imageInfo.tiling = tiling;
	/*
		VK_IMAGE_TILING_LINEARpixels：纹素像我们的数组一样按行主序排列
		VK_IMAGE_TILING_OPTIMAL：纹理元素按照实现定义的顺序进行布局，以实现最佳访问
	*/
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	if (vkCreateImage(logicalDev, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	// 为图像分配内存
	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(logicalDev, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = vk::VulkanInit::findMemoryType(physicalDev, memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(logicalDev, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(logicalDev, image, imageMemory, 0);
}

void VulkanInit::createImageView(VkDevice& logicalDev, VkImage image, VkFormat format, VkImageAspectFlags aspectMask, VkImageView& imageView)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectMask; //VK_IMAGE_ASPECT_COLOR_BIT
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(logicalDev, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}
}

void VulkanInit::transitionImageLayout(VkDevice &dev, VkQueue &queue, VkCommandPool &cp, VkImage &image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) 
{
	VkCommandBuffer commandBuffer = vk::VulkanInit::beginSingleTimeCommands(dev, cp);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	vk::VulkanInit::endSingleTimeCommands(dev, queue, commandBuffer, cp);
}

/**************************Command Buffer*********************************/
VkCommandBuffer VulkanInit::beginSingleTimeCommands(VkDevice &device ,VkCommandPool &cmdPool) {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = {};
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Could not create one-time command buffer!");
	}

	return commandBuffer;
}

void VulkanInit::endSingleTimeCommands(VkDevice& device, VkQueue &queue, VkCommandBuffer &commandBuffer, VkCommandPool &cmdPool) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);

	vkFreeCommandBuffers(device, cmdPool, 1, &commandBuffer);
}



uint32_t VulkanInit::findMemoryType(VkPhysicalDevice &physicalDev, uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDev, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
	{
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw  std::runtime_error("failed to find suitable memory type!");
}

QueueFamilyIndices VulkanInit::findQueueFamilies(VkPhysicalDevice &devices, VkSurfaceKHR &surface)
{
	QueueFamilyIndices indice;

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(devices, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamily(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(devices, &queueFamilyCount, queueFamily.data());

	int index = 0;

	for (const auto qfamily : queueFamily)
	{
		if (qfamily.queueCount <= 0) continue;
		//队列支持图形处理命令
		if ((qfamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (qfamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && qfamily.queueCount > 0)
		{
			indice.graphicAndComputeFamily = index;
			indice.graphicsFamily = index;
		}

		//判定队列族编号对应的队列族下的队列是否支持将图像呈现到窗口上
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(devices, index, surface, &presentSupport);

		if (qfamily.queueCount > 0 && presentSupport)
		{
			indice.presentFamily = index;
		}

		if (indice.isComplete())
		{
			break;
		}
		index++;
	}
	//选择physical device时需要作为判定条件，不能强行终止程序
	//if (!indice.isComplete())
	//{
	//	throw std::runtime_error("No suitable queue .");
	//}

	return indice;
}


VkShaderModule VulkanInit::createShaderModule(VkDevice &device, const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module .");
	}

	return shaderModule;
}