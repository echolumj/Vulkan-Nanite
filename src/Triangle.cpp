#include "Triangle.h"
#include <iostream>
#include <fstream>
#include <random>
#include <ctime>
#include <array>

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif // NDEBUG


//check whether the physical device support the required extensions 
const std::vector<const char*> requireExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};


//call back function
//定义为静态函数，才可以做回调函数
static void framebufferResizeCallback(GLFWwindow * window, int width, int height)
{
	auto app = reinterpret_cast<Triangle*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}


//interact info / mouse
	// 鼠标状态变量
bool isLeftMousePressed = false, isRightMousePressed = false, isScroll = false;
double lastMouseX = 0.0, lastMouseY = 0.0, transzOffset = 0.0;


static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			isLeftMousePressed = true;
			glfwGetCursorPos(window, &lastMouseX, &lastMouseY); // 记录按下时的鼠标位置
		}
		else if (action == GLFW_RELEASE) {
			isLeftMousePressed = false;

		}
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS) {
			isRightMousePressed = true;
			glfwGetCursorPos(window, &lastMouseX, &lastMouseY); // 记录按下时的鼠标位置
		}
		else if (action == GLFW_RELEASE) {
			isRightMousePressed = false;

		}
	}
}


static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	transzOffset = yoffset;
	isScroll = true;
}

// read spv file code
static std::vector<char> readFile(const std::string& filename)
{
	//ate: Start reading at the end of the file
	//binary: Read the file as binary file(avoid text transformations)
	std::ifstream file(filename, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	return buffer;
}

Triangle::Triangle()
{
	//检测窗口大小变化，改变帧缓冲大小
	this->framebufferResized = false;
	this->physicalDevice = VK_NULL_HANDLE;

	this->currentFrame = 0;
}

//public part
void Triangle::run(void)
{
	window_init();
	vulkan_init();
	
	//UI
	_ui = new UI(physicalDevice, logicalDevice, surface, swapChainImageFormat, swapChainExtent);
	_ui->init(window, instance, graphicsQueue, swapChainImageViews);

	main_loop();
	clean_up();
}

std::vector<std::string> filePaths;

static void dropCallback(GLFWwindow* window, int count, const char** paths)
{
	int i;
	filePaths.clear();
	filePaths.resize(count);
	for (i = 0; i < count; i++)
	{
		filePaths[i] = paths[i];
		std::cout << paths[i] << std::endl; //test
	}
	i = 0;
}

//private part
void Triangle::window_init(void)
{
	glfwInit(); //配置GLFW
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //显示设置，阻止自动创建Opengl上下文
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);//窗口大小不可变

	window = glfwCreateWindow(WIDTH, HEIGHT, "vulkan", nullptr, nullptr);

	//creat window fail
	if (window == NULL)
	{
		std::cout << "Failed to creat GLFW	window" << "\n";
		glfwTerminate();
		return;
	}
	glfwSetWindowUserPointer(window, this);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetScrollCallback(window, ScrollCallback);
	glfwSetDropCallback(window, dropCallback);
}

void Triangle::vulkan_init(void)
{
	vk::VulkanInit::createInstance(instance, debugMessenger, enableValidationLayers);
	vk::VulkanInit::createSurface(instance, window, surface);
	vk::VulkanInit::pickPhysicalDevice(physicalDevice, instance, surface, requireExtensions);

	logicalDevice_create();
	swapChain_create();
	imageView_create();
	commandPool_create();
	commondBuffers_create();

	depthResources_create();
	renderPass_create();

	_sceneManager = new scene::SceneManager(physicalDevice, logicalDevice);
	ubo.model = glm::mat4(1.0);
	
	graphicsPipline_create();
	framebuffer_create();
	syncObjects_create();
}


void Triangle::main_loop(void)
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		if(isLeftMousePressed || isRightMousePressed || isScroll)
			sceneInteract();

		_ui->draw(ubo);

		if (filePaths.size() > 0)
		{
			for (int i = 0; i < filePaths.size(); ++i)
			{
				curModelId = _sceneManager->addModel(filePaths[i]);
			}
			filePaths.clear();
		}
		drawFrame();
		//★vkQueueWaitIdle(presentQueue);
	}
	//all of the operations in drawFrame are asynchronous
	vkDeviceWaitIdle(logicalDevice);
}

void Triangle::drawFrame(void)
{
	//fence initized as true
	vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

	//1.Acquiring an image from the swap chain
	uint32_t imageIndex;
	VkResult result =  vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		swapCahin_recreate();
		return;
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
	{
		vkWaitForFences(logicalDevice, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
	}

	vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);

	imagesInFlight[imageIndex] = inFlightFences[currentFrame];

	//2.Submitting the command buffer
	vkResetCommandBuffer(commandBuffers[currentFrame], 0);
	recordCommandBuffer(commandBuffers[currentFrame], imageIndex, currentFrame);
	_ui->recordUICommandBuffer(event[currentFrame], imageIndex, currentFrame);

	std::vector<VkSemaphore> waitSemaphores{ imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::array<VkCommandBuffer, 2> cmdBuffers = { commandBuffers[currentFrame], _ui->getCommandBuffer(currentFrame)};
	std::vector<VkSemaphore> signalSemaphores{ renderFinishedSemaphores[currentFrame] };

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = static_cast<uint32_t>( waitSemaphores.size());
	submitInfo.pWaitSemaphores = waitSemaphores.data();
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 2;
	submitInfo.pCommandBuffers = cmdBuffers.data();
	submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	submitInfo.pSignalSemaphores = signalSemaphores.data();

	// submit the command buffer to the graphics queue 
	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
	presentInfo.pWaitSemaphores = signalSemaphores.data();

	std::vector<VkSwapchainKHR> swapChains{ swapChain };
	presentInfo.swapchainCount = static_cast<uint32_t>(swapChains.size());
	presentInfo.pSwapchains = swapChains.data();
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = nullptr;

	result = vkQueuePresentKHR(presentQueue, &presentInfo);

	//we will also recreate the swap chain if it is suboptimal, because we want the best possible result.
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
	{
		//此时以及执行了present，不需要return
		framebufferResized = false;//consistent
		swapCahin_recreate();
	}
	else if (result != VK_SUCCESS)
	{
		throw std::runtime_error("failed to present swap chain image!");
	}

	//为什么要把这行代码放在这而不是循环中？
	//因为会存在这个函数中未执行到这句话，而转去重建swapchain，可能存在被丢弃需要重新显示的image
	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Triangle::swapCahin_recreate() 
{
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	//part1: wait device idle 
	vkDeviceWaitIdle(logicalDevice);

	//part2:clean up old swap chain
	cleanupSwapChain();

	//part3:recreate swap chain
	swapChain_create();
	imageView_create();
	depthResources_create();
	framebuffer_create();

	//UI
	_ui->cleanupUIResources();
	_ui->recreateUISwapChain(imageCount, swapChainImageViews);
}

void Triangle::cleanupSwapChain(void)
{
	//1.FrameBuffer 
	for (auto framebuffer : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
	}

	//2.Swap Chain Images View
	for (auto imageView : swapChainImageViews)
	{
		vkDestroyImageView(logicalDevice, imageView, nullptr);
	}

	vkDestroyImage(logicalDevice, depthImage, nullptr);
	vkFreeMemory(logicalDevice, depthImageMemory, nullptr);
	vkDestroyImageView(logicalDevice, depthImageView, nullptr);

	//3.Swap Chain
	vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
}

void Triangle::clean_up(void)
{
	//vkFreeCommandBuffers(logicalDevice, commandPool, commandBuffers.size(), commandBuffers.data());
	cleanupSwapChain();

	delete _ui;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
		vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
		vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
		vkDestroyEvent(logicalDevice, event[i], nullptr);
	}
	/*vkDestroyBuffer(logicalDevice, vertexBuffer, nullptr);
	vkFreeMemory(logicalDevice, vertexBufferMem, nullptr);
	vkDestroyBuffer(logicalDevice, indiceBuffer, nullptr);
	vkFreeMemory(logicalDevice, indiceBufferMem, nullptr);*/

	if (_sceneManager)
		delete _sceneManager;

	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
		
	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

	vkDestroyDevice(logicalDevice, nullptr);

	if (enableValidationLayers)
		vk::VulkanInit::DestoryDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	
	//要确保surface在instance之前被销毁
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}


void Triangle::logicalDevice_create(void)
{
	vk::QueueFamilyIndices indice = vk::VulkanInit::findQueueFamilies(physicalDevice, surface);

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
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requireExtensions.size());//static_cast<uint32_t>(requireExtensions.size());
	createInfo.ppEnabledExtensionNames = requireExtensions.data();

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(logicalDevice, indice.graphicAndComputeFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, indice.graphicAndComputeFamily.value(), 0, &computeQueue);
	vkGetDeviceQueue(logicalDevice, indice.presentFamily.value(), 0, &presentQueue);
}

void Triangle::swapChain_create(void)
{
	vk::SwapChainSupportDetails swapChainDetails = vk::VulkanInit::querySwapChainSupport(physicalDevice, surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainDetails.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainDetails.presentModes);

	VkExtent2D swapExtent = chooseSwapExtent(swapChainDetails.capabilities);

	//swapExtent.height = max(swapChainDetails.capabilities.minImageExtent.height, min(swapChainDetails.capabilities.maxImageExtent.height, HEIGHT));
	//swapExtent.width = max(swapChainDetails.capabilities.minImageExtent.width, min(swapChainDetails.capabilities.maxImageExtent.width, WIDTH));

	imageCount = swapChainDetails.capabilities.minImageCount + 1;

	if (swapChainDetails.capabilities.maxImageCount > 0 && imageCount > swapChainDetails.capabilities.maxImageCount)
	{
		imageCount = swapChainDetails.capabilities.maxImageCount;
	}

	//create swap chain part
	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount; //why add 1
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = swapExtent;
	createInfo.imageArrayLayers = 1;//specifies the amount of layers each image consists of
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	vk::QueueFamilyIndices indices = vk::VulkanInit::findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

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

	if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

	swapChainExtent = swapExtent;
	swapChainImageFormat = surfaceFormat.format;
}

void Triangle::imageView_create(void)
{
	swapChainImageViews.resize(swapChainImages.size());

	//create an image view for each image
	for (int i = 0; i < swapChainImages.size(); i++)
	{
		VkImageViewCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;

		//这里选择默认，也可以通过将所有的通道映射到一个通道，实现黑白纹理
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		//表示用途，这里只做颜色使用，也可以做深度使用
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.layerCount = 1;
		createInfo.subresourceRange.levelCount = 1;

		if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i])!= VK_SUCCESS)
		{
			throw std::runtime_error("failed to create image view.");
		}
	}
}

void Triangle::renderPass_create(void)
{
	//only one color buffer
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //multi sample disable
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//stencil buffer 没有用到
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	//the layout of the pixels in memory
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//depth
	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthImageFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT; //multi sample disable
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	//color
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; //index of attachment
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	//depth
	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1; //index of attachment
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//作为图形管线
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0; //index of subpass
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void Triangle::graphicsPipline_create(void)
{
	auto vertexShaderCode = readFile(RELATIVE_PATH + std::string("/spvs/vert.spv"));
	auto fragShaderCode = readFile(RELATIVE_PATH + std::string("/spvs/frag.spv"));

	VkShaderModule vertShaderModule = vk::VulkanInit::createShaderModule(logicalDevice, vertexShaderCode);
	VkShaderModule fragmentShaderModule = vk::VulkanInit::createShaderModule(logicalDevice, fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main"; // entry points

	VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragmentShaderModule;
	fragShaderStageInfo.pName = "main";

	//step 1:prepare Shader Stage
	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
	
	//step 2:prepare Vertex Input State
	auto bindingDesc = Vertex::getBindingDescription();
	auto attributeDesc = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDesc;
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 3;
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDesc.data();

	//step 3:input assembly 
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo{};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	//step 4:Viewports and scissors
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor{};
	scissor.extent = swapChainExtent;//show all
	scissor.offset = { 0, 0 };

	VkPipelineViewportStateCreateInfo viewportCreateInfo{};
	viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportCreateInfo.viewportCount = 1;
	viewportCreateInfo.pViewports = &viewport;
	viewportCreateInfo.scissorCount = 1;
	viewportCreateInfo.pScissors = &scissor;

	//step 5:Rasterizer
	VkPipelineRasterizationStateCreateInfo rasterCreateInfo{};
	rasterCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterCreateInfo.depthClampEnable = VK_FALSE;
	rasterCreateInfo.rasterizerDiscardEnable = VK_FALSE;//★
	rasterCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterCreateInfo.lineWidth = 1.0f;
	rasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterCreateInfo.depthBiasEnable = VK_FALSE;

	//step 5:Multisample 
	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo{};
	multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
	multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;


	//step 6:Depth and stencil testing 
	VkPipelineDepthStencilStateCreateInfo depth_info = {};
	depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_info.depthTestEnable = VK_TRUE;
	depth_info.depthWriteEnable = VK_TRUE;
	depth_info.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_info.depthBoundsTestEnable = VK_FALSE;
	depth_info.stencilTestEnable = VK_FALSE;
	
	//step 7:Color blending
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo colorBlendState{};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachment;
	colorBlendState.blendConstants[0] = 0.0f;
	colorBlendState.blendConstants[1] = 0.0f;
	colorBlendState.blendConstants[2] = 0.0f;
	colorBlendState.blendConstants[3] = 0.0f;

	//step 8:Dynamic state 
	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
 
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;


	//pushconstant 
	VkPushConstantRange pushConstant = {};
	pushConstant.offset = 0;
	pushConstant.size = sizeof(base::UniformBufferObject);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	//step 9:Pipeline layout 
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
	         
	if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, NULL, &pipelineLayout) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create graphics pipeline . ");
	}

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;//可编程的stage number
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputCreateInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	pipelineInfo.pViewportState = &viewportCreateInfo;
	pipelineInfo.pRasterizationState = &rasterCreateInfo;
	pipelineInfo.pMultisampleState = &multisampleCreateInfo;
	pipelineInfo.pDepthStencilState = &depth_info;//optional
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.pDynamicState = &dynamicState;// &dynamicState; //optional
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;//index of used subpass

	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;

	if (vkCreateGraphicsPipelines(logicalDevice, nullptr, 1, &pipelineInfo, nullptr, &graphicsPipeline))
	{
		throw std::runtime_error("failed to create graphics pipeline!");
	}

	//destroy the shader modules again as soon as pipeline creation is finished
	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragmentShaderModule, nullptr);
}


void Triangle::framebuffer_create(void)
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (int i = 0; i < swapChainImageViews.size(); i++)
	{
		//specify the VkImageView objects that should be bound to the respective
		//attachment descriptions in the render pass pAttachment array
		VkImageView attachment[] = { swapChainImageViews[i], depthImageView};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = attachment;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void Triangle::commandPool_create(void)
{
	vk::QueueFamilyIndices queueFamilyIndices = vk::VulkanInit::findQueueFamilies(physicalDevice, surface);

	VkCommandPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
	poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Optional  neither

	if (vkCreateCommandPool(logicalDevice, &poolCreateInfo, nullptr, &commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}

}

void Triangle::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame)
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = 0;
	beginInfo.pInheritanceInfo = nullptr;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("failed to begin recording command buffer!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

	renderPassInfo.renderArea.offset = { 0,0 };
	renderPassInfo.renderArea.extent = swapChainExtent;

	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
	clearValues[1].depthStencil = { 1.0f, 0 };
	renderPassInfo.clearValueCount = 2;
	renderPassInfo.pClearValues = clearValues.data();

	//The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 10.0f);
	glm::vec3 focusPos = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 front = focusPos - cameraPos;
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 left = glm::cross(up, front);

	//base::UniformBufferObject ubo;
	ubo.model = _sceneManager->getModelMatrix();
	ubo.projection = _sceneManager->getProjMatrix(WIDTH, HEIGHT);
	ubo.view = _sceneManager->getViewMatrix();

	vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(ubo), &ubo);

	//The second parameter specifies if the pipeline object is a graphics or compute pipeline. 
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(swapChainExtent.width);
	viewport.height = static_cast<float>(swapChainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	if (_sceneManager && _sceneManager->getModelSize() > 0)
	{
		// After a transfer operation, ensure data is available for vertex shader:
		//VkBufferMemoryBarrier bufferBarrier = {};
		//bufferBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		//bufferBarrier.pNext = nullptr;
		//bufferBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;  // 屏障之前是传输写入操作
		//bufferBarrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;  // 屏障之后是顶点属性读取操作
		//bufferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // 不需要队列族所有权转移
		//bufferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // 不需要队列族所有权转移
		//bufferBarrier.buffer = vertexBuffer;  // 需要同步的缓冲区对象
		//bufferBarrier.offset = 0;       // 从缓冲区的起始位置开始
		//bufferBarrier.size = VK_WHOLE_SIZE;  // 同步整个缓冲区

		//// 将屏障插入到命令缓冲区中
		//vkCmdPipelineBarrier(
		//	commandBuffer,  // 当前命令缓冲区
		//	VK_PIPELINE_STAGE_TRANSFER_BIT,  // 源阶段：传输操作
		//	VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,  // 目标阶段：顶点输入
		//	0,  // 依赖标志
		//	0, nullptr,  // 内存屏障数量及指针
		//	1, &bufferBarrier,  // 缓冲区屏障数量及指针
		//	0, nullptr  // 图像屏障数量及指针
		//);
		//std::vector<VkBuffer> vertexBuffers(_sceneManager->getModelSize());
		VkDeviceSize offsets[] = { 0 };
		for (int i = 0; i < _sceneManager->getModelSize(); ++i)
		{
			scene::Model model = _sceneManager->getModel(i);
			//vertexBuffers[i] = ;
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &(model.vertex.buffer), offsets);

			vkCmdBindIndexBuffer(commandBuffer, model.indice.buffer, 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(commandBuffer, model.indice.count, 1, 0, 0, 0);
		}
		
		//vkCmdBindVertexBuffers(commandBuffer, 0, vertexBuffers.size(), vertexBuffers.data(), offsets);
		//
		//vkCmdBindIndexBuffer(commandBuffer, model.indice.buffer, 0, VK_INDEX_TYPE_UINT16);
		//vkCmdDrawIndexed(commandBuffer, model.indice.count, 1, 0, 0, 0);

	}

	//vkCmdDraw(commandBuffer, 3, 1, 0, 0);
	//vkCmdDrawIndexed(commandBuffer, modelIndicesNum, 1, 0, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	vkCmdSetEvent(commandBuffer, event[curFrame], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to record command buffer!");
	}
}

void Triangle::commondBuffers_create(void)
{
	//command buffer的数量和swapChainFramebuffersframe buffer的数量一致
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

void Triangle::syncObjects_create(void)
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	event.resize(MAX_FRAMES_IN_FLIGHT);

	imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo{}; 
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;//initial state of fence = signed

	VkEventCreateInfo eventInfo = {};
	eventInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
	eventInfo.flags = VK_EVENT_CREATE_DEVICE_ONLY_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS||
			vkCreateFence(logicalDevice, &fenceCreateInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS ||
			vkCreateEvent(logicalDevice, &eventInfo, nullptr, &event[i]))
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

VkFormat Triangle::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

VkFormat Triangle::findDepthFormat() {
	return findSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

void Triangle::depthResources_create(void)
{
	VkFormat depthFormat = findDepthFormat();
	depthImageFormat = depthFormat;
	vk::VulkanInit::createImage(physicalDevice, logicalDevice, swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	vk::VulkanInit::createImageView(logicalDevice, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depthImageView);

	vk::VulkanInit::transitionImageLayout(logicalDevice, graphicsQueue, commandPool, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

}


VkSurfaceFormatKHR Triangle::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
	for (const auto& format : formats)
	{
		if (format.format == VK_FORMAT_B8G8R8_UNORM && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			return format;
	}
	return formats[0];
}

//目前有的driver不支持VK_PRESENT_MODE_FIFO_KHR，所以当VK_PRESENT_MODE_MAILBOX_KHR不可用时，
//我们倾向于VK_PRESENT_MODE_IMMEDIATE_KHR
VkPresentModeKHR Triangle::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;
	for (const auto& mode : presentModes)
	{
		if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
			return mode;
		else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
			bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	}
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D Triangle::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = static_cast<uint32_t>(std::fmax(capabilities.minImageExtent.width, std::fmin(capabilities.maxImageExtent.width, actualExtent.width)));
		actualExtent.height = static_cast<uint32_t>(std::fmax(capabilities.minImageExtent.height, std::fmin(capabilities.maxImageExtent.height, actualExtent.height)));

		return actualExtent;
	}
}


void Triangle::sceneInteract(void)
{
	glm::vec3 rotationAngles(0.0f);
	glm::vec3 translate(0.0f);

	double currentMouseX, currentMouseY;
	glfwGetCursorPos(window, &currentMouseX, &currentMouseY);

	// 计算鼠标移动的偏移量
	double deltaX = currentMouseX - lastMouseX;
	double deltaY = currentMouseY - lastMouseY;

	// 更新鼠标位置
	lastMouseX = currentMouseX;
	lastMouseY = currentMouseY;

	if (isLeftMousePressed)
	{
		// 根据偏移量更新旋转角度
		rotationAngles.y = static_cast<float>(deltaX) * 0.01f; // 绕Y轴旋转
		rotationAngles.x = static_cast<float>(deltaY) * 0.01f; // 绕X轴旋转
	}
	else if(isRightMousePressed)
	{
		//auto z = _sceneManager->getModelMatrix()[2][3];
		translate = glm::vec3(static_cast<float>(deltaX) * 0.01f, static_cast<float>(deltaY) * 0.01f, 0.0);
		transzOffset = 0.0f;
	}
	else if (isScroll)
	{
		translate.z = -transzOffset;
		isScroll = false;
	}

	_sceneManager->setModelMatrix(rotationAngles, translate);
}
