#include "vulkanInit.h"
#include <iostream>
#include<set>

using namespace vk;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	//去除VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT（正常的诊断信息）
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

	VkApplicationInfo appInfo{};//{}初始化必不可少，结构体内存在指针变量
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

	//置于if语句之外，以确保它在vkCreateInstance 调用之前不会被销毁
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
	//用vulkan创建surface
	//配置相关信息
	//VkWin32SurfaceCreateInfoKHR  surfaceCreateInfo{};
	//surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	//surfaceCreateInfo.hwnd = glfwGetWin32Window(window);//获取窗口handle
	//surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

	//if (vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface))
	//{
	//	throw std::runtime_error(" failed to create window surface .");
	//}


	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}

}

void VulkanInit::createLogicalDevice(VkDevice& dev, VkPhysicalDevice& pdev, VkSurfaceKHR& surface, const std::vector<const char*>& extension, bool enableValidationLayers = false)
{
	vk::QueueFamilyIndices indice = vk::VulkanInit::findQueueFamilies(pdev, surface);

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indice.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;
	float priorities = 1.0f;
	queueCreateInfo.pQueuePriorities = &priorities;

	//创建逻辑设备时，我们需要指明要使用物理设备所支持的特性中的哪一些特性
	VkPhysicalDeviceFeatures deviceFeatures = {};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;

	//在choose physical device 中已经检查过extension的支持
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extension.size());//static_cast<uint32_t>(requireExtensions.size());
	createInfo.ppEnabledExtensionNames = extension.data();

	const std::vector<const char*> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
	};

	if (enableValidationLayers)
	{

		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(pdev, &createInfo, nullptr, &dev) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	//vkGetDeviceQueue(dev, indice.graphicAndComputeFamily.value(), 0, &graphicsQueue);
	//vkGetDeviceQueue(dev, indice.graphicAndComputeFamily.value(), 0, &computeQueue);
	//vkGetDeviceQueue(dev, indice.presentFamily.value(), 0, &presentQueue);
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

	//选择策略：选取最先满足条件的physicaldevice
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

void VulkanInit::createSwapChain(SwapChainInfo &swapchaininfo, VkSwapchainKHR &swapChain, VkPhysicalDevice &pdev, VkDevice &dev, VkSurfaceKHR &surface, GLFWwindow *window)
{
	vk::SwapChainSupportDetails swapChainDetails = vk::VulkanInit::querySwapChainSupport(pdev, surface);

	//choose SwapSurface Format
	VkSurfaceFormatKHR surfaceFormat = swapChainDetails.formats[0];
	for (const auto& format : swapChainDetails.formats)
	{
		if (format.format == VK_FORMAT_B8G8R8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			surfaceFormat = format;
	}

	//chooseSwapPresentMode
	VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& mode : swapChainDetails.presentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			presentMode = mode;
			break;
		}
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}

	//chooseSwapExtent
	VkExtent2D swapExtent;
	if (swapChainDetails.capabilities.currentExtent.width != UINT32_MAX) {
		swapExtent = swapChainDetails.capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		swapExtent.width = static_cast<uint32_t>(std::fmax(swapChainDetails.capabilities.minImageExtent.width, std::fmin(swapChainDetails.capabilities.maxImageExtent.width, actualExtent.width)));
		swapExtent.height = static_cast<uint32_t>(std::fmax(swapChainDetails.capabilities.minImageExtent.height, std::fmin(swapChainDetails.capabilities.maxImageExtent.height, actualExtent.height)));
	}


	swapchaininfo.imageCount = swapChainDetails.capabilities.minImageCount + 1;

	if (swapChainDetails.capabilities.maxImageCount > 0 && swapchaininfo.imageCount > swapChainDetails.capabilities.maxImageCount)
	{
		swapchaininfo.imageCount = swapChainDetails.capabilities.maxImageCount;
	}

	//create swap chain part
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = swapchaininfo.imageCount; //why add 1
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapExtent;
	createInfo.imageArrayLayers = 1;//specifies the amount of layers each image consists of
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	vk::QueueFamilyIndices indices = vk::VulkanInit::findQueueFamilies(pdev, surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	//图形渲染和image呈现在surface的队列族不一致：并发模式
	if (indices.presentFamily != indices.graphicsFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainDetails.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(dev, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(dev, swapChain, &swapchaininfo.imageCount, nullptr);
	swapchaininfo.images.resize(swapchaininfo.imageCount);
	vkGetSwapchainImagesKHR(dev, swapChain, &swapchaininfo.imageCount, swapchaininfo.images.data());

	swapchaininfo.extent = swapExtent;
	swapchaininfo.format = surfaceFormat.format;

	//swapchain image view 
	swapchaininfo.imageViews.resize(swapchaininfo.images.size());

	//create an image view for each image
	for (int i = 0; i < swapchaininfo.imageViews.size(); i++)
	{
		vk::VulkanInit::createImageView(dev, swapchaininfo.images[i], swapchaininfo.format, VK_IMAGE_ASPECT_COLOR_BIT, swapchaininfo.imageViews[i]);
	}
}

void VulkanInit::createCommandPool(VkCommandPool &cp, VkDevice &dev, uint32_t fIndex)
{
	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.queueFamilyIndex = fIndex;
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional  neither

	if (vkCreateCommandPool(dev, &poolCreateInfo, nullptr, &cp) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}

}

void VulkanInit::createCommondBuffers(std::vector<VkCommandBuffer>& cbs, VkDevice& dev, VkCommandPool &cp, VkCommandBufferLevel level)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = cp;
	allocInfo.level = level;
	allocInfo.commandBufferCount = (uint32_t)cbs.size();

	if (vkAllocateCommandBuffers(dev, &allocInfo, cbs.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void VulkanInit::createRenderPass(VkRenderPass &renderPass, VkDevice &device, std::vector<VkAttachmentDescription> &colorAttachments, VkAttachmentDescription &depthAttachment, std::vector<VkSubpassDescription> &subpasses, std::vector<VkSubpassDependency> &dependencies)
{
	std::vector<VkAttachmentDescription> attachments = colorAttachments;
	attachments.emplace_back(depthAttachment);
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = subpasses.size();
	renderPassInfo.pSubpasses = subpasses.data();
	renderPassInfo.dependencyCount = dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void VulkanInit::createFrameBuffer(VkFramebuffer &fb, VkDevice &dev, VkRenderPass &rp, std::vector<VkImageView> &cimage, VkImageView &dimage, VkExtent2D extent)
{
	//specify the VkImageView objects that should be bound to the respective
	//attachment descriptions in the render pass pAttachment array
	std::vector<VkImageView> attachments = cimage;
	attachments.emplace_back(dimage);

	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = rp;
	framebufferInfo.attachmentCount = attachments.size();
	framebufferInfo.pAttachments = attachments.data();
	framebufferInfo.height = extent.height;
	framebufferInfo.width = extent.width;
	framebufferInfo.layers = 1;

	if (vkCreateFramebuffer(dev, &framebufferInfo, nullptr, &fb) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create framebuffer!");
	}

}

/**************************Class Type*********************************/
VkAttachmentDescription VulkanInit::AttachmentDescription(VkFormat format, VkSampleCountFlagBits samples, VkAttachmentLoadOp loadOp, VkAttachmentStoreOp storeOp, VkImageLayout initialLayout, VkImageLayout finalLayout)
{
	VkAttachmentDescription description = {};
	description.format = format;
	description.samples = samples; //multi sample disable
	description.loadOp = loadOp;
	description.storeOp = storeOp;
	description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//the layout of the pixels in memory
	description.initialLayout = initialLayout;
	description.finalLayout = finalLayout;

	return description;
}

VkSubpassDescription VulkanInit::Subpass(std::vector<VkAttachmentDescription>& colorAttachments, VkAttachmentDescription& depthAttachment)
{
	//color
	//temp [TODO]
	static std::vector<VkAttachmentReference> attachmentRefs(colorAttachments.size());
	for (int i = 0; i < colorAttachments.size(); ++i)
	{
		attachmentRefs[i] = {};
		attachmentRefs[i].attachment = i; //index of attachment
		attachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	//depth
	//temp [TODO]
	static VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = colorAttachments.size();
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//作为图形管线
	subpass.colorAttachmentCount = attachmentRefs.size();
	subpass.pColorAttachments = attachmentRefs.data();
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	return subpass;
}

VkSubpassDependency VulkanInit::SubpassDependency(uint32_t srcSubpass, uint32_t dstSubpass, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask)
{
	VkSubpassDependency dependency{};
	dependency.srcSubpass = srcSubpass;
	dependency.dstSubpass = dstSubpass; //index of subpass
	dependency.srcStageMask = srcStageMask;
	dependency.srcAccessMask = srcAccessMask;
	dependency.dstStageMask = dstStageMask;
	dependency.dstAccessMask = dstAccessMask;

	return dependency;
}

VkPipelineShaderStageCreateInfo VulkanInit::PipelineShaderStage(VkShaderStageFlagBits stage, VkShaderModule module)
{
	VkPipelineShaderStageCreateInfo shaderStageInfo{};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.stage = stage;
	shaderStageInfo.module = module;
	shaderStageInfo.pName = "main"; // entry points

	return shaderStageInfo;
}

VkPipelineVertexInputStateCreateInfo VulkanInit::PipelineVertexInputState(std::vector<VkVertexInputBindingDescription>& bindingDescs, std::vector<VkVertexInputAttributeDescription>& attrDescs)
{
	VkPipelineVertexInputStateCreateInfo inputCreateInfo{};
	inputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	inputCreateInfo.vertexBindingDescriptionCount = bindingDescs.size();
	inputCreateInfo.pVertexBindingDescriptions = bindingDescs.data();
	inputCreateInfo.vertexAttributeDescriptionCount = attrDescs.size();
	inputCreateInfo.pVertexAttributeDescriptions = attrDescs.data();

	return inputCreateInfo;
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

	//作为选择physical device的判定条件之一，不需要抛出程序错误，强制中断程序
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

	//★print all required extensions
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
			//★print out all names of supported layer 
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

	// 独显 + geometryShader
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

void VulkanInit::createImageView(VkDevice& logicalDev, VkImage &image, VkFormat format, VkImageAspectFlags aspectMask, VkImageView& imageView)
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