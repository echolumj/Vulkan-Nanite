#include "vulkanInit.h"
#include <iostream>

#include<set>

using namespace vk;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	//ȥ��VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT�������������Ϣ��
	if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

VkResult VulkanInit::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func == nullptr)
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
	return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}


void VulkanInit::DestoryDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}


void VulkanInit::createInstance(VkInstance &instance, VkDebugUtilsMessengerEXT &debugMessenger, bool enableValidationLayers)
{
	const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
	};


	if (enableValidationLayers && !vk::VulkanInit::CheckLayerSupport(validationLayers)) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	VkApplicationInfo appInfo{};//{}��ʼ���ز����٣��ṹ���ڴ���ָ�����
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = vk::VulkanInit::getRequiredExtensions(enableValidationLayers);
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	//����if���֮�⣬��ȷ������vkCreateInstance ����֮ǰ���ᱻ����
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		//set debug information call back
		debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugCreateInfo.pfnUserCallback = debugCallback;

		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.ppEnabledLayerNames = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}

	if (!enableValidationLayers) return;

	if (vk::VulkanInit::CreateDebugUtilsMessengerEXT(instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}

void VulkanInit::createSurface(VkInstance &instance, GLFWwindow *window, VkSurfaceKHR &surface)
{
	//��vulkan����surface
	//���������Ϣ
	//VkWin32SurfaceCreateInfoKHR  surfaceCreateInfo{};
	//surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	//surfaceCreateInfo.hwnd = glfwGetWin32Window(window);//��ȡ����handle
	//surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

	//if (vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface))
	//{
	//	throw std::runtime_error(" failed to create window surface .");
	//}


	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}

}

void VulkanInit::pickPhysicalDevice(VkPhysicalDevice &pdev, VkInstance &instance, VkSurfaceKHR& surface, const std::vector<const char*>& extensions)
{
	uint32_t deviceCount;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		throw std::runtime_error("failed to find physical device .");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	//ѡ����ԣ�ѡȡ��������������physicaldevice
	for (const auto phydevice : devices)
	{
		if (vk::VulkanInit::isDeviceSuitable(phydevice, surface, extensions))
		{
			pdev = phydevice;
			break;
		}
	}

	if (pdev == VK_NULL_HANDLE)
	{
		throw std::runtime_error("no suitable device.");
	}

	//test code: tools
	PFN_vkGetPhysicalDeviceToolPropertiesEXT vkGetPhysicalDeviceToolPropertiesEXT = NULL;
	vkGetPhysicalDeviceToolPropertiesEXT = (PFN_vkGetPhysicalDeviceToolPropertiesEXT)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceToolPropertiesEXT");
	if (vkGetPhysicalDeviceToolPropertiesEXT == NULL)
		throw std::runtime_error("get instance function failed");

	uint32_t toolCount = 0;
	auto result = vkGetPhysicalDeviceToolPropertiesEXT(pdev, &toolCount, nullptr);
	if (toolCount <= 0 || result != VK_SUCCESS)
		throw std::runtime_error("tool of physical device is none");

	std::vector<VkPhysicalDeviceToolPropertiesEXT> tools(toolCount);
	result = vkGetPhysicalDeviceToolPropertiesEXT(pdev, &toolCount, tools.data());
	if (result != VK_SUCCESS)
		throw std::runtime_error("tool of physical device is none");

	printf("Active tools:\n");
	for (uint32_t i = 0; i < toolCount; i++) {
		printf("Tool Name: %s\n", tools[i].name);
		printf("Description: %s\n", tools[i].description);
		printf("Tool Version: %s\n", tools[i].version);
		printf("Purposes: %u\n", tools[i].purposes); // Use bitmask for purposes
		printf("Layer Name: %s\n", tools[i].layer[0] ? tools[i].layer : "None");
		printf("---------------------------------\n");
	}

}

bool VulkanInit::CheckDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*> &extensions)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtension(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtension.data());

	std::set<std::string> deviceExtensions(extensions.begin(), extensions.end());

	for (const auto& extension : availableExtension)
	{
		deviceExtensions.erase(extension.extensionName);
	}

	//��Ϊѡ��physical device���ж�����֮һ������Ҫ�׳��������ǿ���жϳ���
	//if (!deviceExtensions.empty()) {
	//	throw std::runtime_error("extension not fulfill");
	//}

	return deviceExtensions.empty();
}

vk::SwapChainSupportDetails VulkanInit::querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR &surface)
{
	vk::SwapChainSupportDetails swapChainDetails;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &(swapChainDetails.capabilities));

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		swapChainDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
	if (presentModeCount != 0)
	{
		swapChainDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, swapChainDetails.presentModes.data());
	}
	return swapChainDetails;
}

std::vector<const char*> VulkanInit::getRequiredExtensions(bool enableValidationLayers)
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensionCount + glfwExtensions);

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	//��print all required extensions
	std::cout << "all required extensions" << "\n";
	for (const auto extension : extensions)
		std::cout << extension << "\n";

	return extensions;
}

bool VulkanInit::CheckLayerSupport(const std::vector<const char*> &layers)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> layerProperties(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

	std::cout << "all supported layer names" << "\n";

	for (const char* layerName : layers)
	{
		bool layFound = false;
		for (const auto& layerProperties : layerProperties)
		{
			//��print out all names of supported layer 
			std::cout << layerProperties.layerName << "\n";
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layFound = true;
				break;
			}
		}
		if (!layFound)
			return false;
	}
	return true;
}

bool VulkanInit::isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR& surface, const std::vector<const char*>& extensions)
{
	//VkPhysicalDeviceProperties property;
	//VkPhysicalDeviceFeatures feature;

	//vkGetPhysicalDeviceProperties(devices, &property);
	//vkGetPhysicalDeviceFeatures(devices, &feature);

	// ���� + geometryShader
	//return (property.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && feature.geometryShader;

	vk::QueueFamilyIndices indices = vk::VulkanInit::findQueueFamilies(device, surface);

	bool extensionSupport = vk::VulkanInit::CheckDeviceExtensionSupport(device, extensions);//swapchain
	bool swapChainAdequate = false;

	if (extensionSupport)
	{
		vk::SwapChainSupportDetails swapChainDetails = vk::VulkanInit::querySwapChainSupport(device, surface);
		swapChainAdequate = !(swapChainDetails.formats.empty() || swapChainDetails.presentModes.empty());
	}

	return indices.isComplete() && extensionSupport && swapChainAdequate;
}


/**************************BUFFER********************************/
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
	//Vkͼ����󴴽�
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = static_cast<uint32_t>(width);
	imageInfo.extent.height = static_cast<uint32_t>(height);
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;      //Vulkan֧�ֶ���ͼ���ʽ���������������Ҫ�ڻ�������Ϊ����Ӧ��������һ�µĸ�ʽ�����򿽱�������ʧ�ܡ�(VK_FORMAT_R8G8B8A8_SRGB)
	imageInfo.tiling = tiling;
	/*
		VK_IMAGE_TILING_LINEARpixels�����������ǵ�����һ��������������
		VK_IMAGE_TILING_OPTIMAL������Ԫ�ذ���ʵ�ֶ����˳����в��֣���ʵ����ѷ���
	*/
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.flags = 0;

	if (vkCreateImage(logicalDev, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	// Ϊͼ������ڴ�
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
		//����֧��ͼ�δ�������
		if ((qfamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (qfamily.queueFlags & VK_QUEUE_COMPUTE_BIT) && qfamily.queueCount > 0)
		{
			indice.graphicAndComputeFamily = index;
			indice.graphicsFamily = index;
		}

		//�ж��������Ŷ�Ӧ�Ķ������µĶ����Ƿ�֧�ֽ�ͼ����ֵ�������
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
	//ѡ��physical deviceʱ��Ҫ��Ϊ�ж�����������ǿ����ֹ����
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