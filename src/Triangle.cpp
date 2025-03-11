#include "Triangle.h"
#include <iostream>
#include <fstream>
#include <set>
#include <random>
#include <ctime>
#include <array>

//#include "Gizmo/imoguizmo.hpp"

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif // NDEBUG


const std::vector<Vertex> vertices = {
	{{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f},{1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}}
};

//check whether the physical device support the required extensions 
const std::vector<const char*> requireExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
};

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	//去除VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT（正常的诊断信息）
	if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

//call back function
//定义为静态函数，才可以做回调函数
static void framebufferResizeCallback(GLFWwindow * window, int width, int height)
{
	auto app = reinterpret_cast<Triangle*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
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
	ui_init();
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
	glfwSetDropCallback(window, dropCallback);
}

void Triangle::vulkan_init(void)
{
	instance_create();
	debugMessenger_setUp();
	surface_create();
	physicalDevice_pick();
	logicalDevice_create();
	swapChain_create();
	imageView_create();
    renderPass_create();

	commandPool_create();
	commondBuffers_create();

	//basic triangle
	//vertexBuffer_create();
	//vertexAndIndiceBuffer_create();
	_sceneManager = new scene::SceneManager(physicalDevice, logicalDevice);

	
	graphicsPipline_create();
	framebuffer_create();
	syncObjects_create();
}

void Triangle::ui_init(void)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Initialize some DearImgui specific resources
	createUIDescriptorPool();
	createUIRenderPass();
	createUICommandPool(&uiCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	createUICommandBuffers();
	createUIFramebuffers();

	QueueFamilyIndices indice = findQueueFamilies(physicalDevice);

	// Provide bind points from Vulkan API
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = logicalDevice;
	init_info.QueueFamily = indice.graphicsFamily.value();
	init_info.Queue = graphicsQueue;
	init_info.DescriptorPool = uiDescriptorPool;
	init_info.MinImageCount = imageCount;
	init_info.ImageCount = imageCount;
	ImGui_ImplVulkan_Init(&init_info, uiRenderPass);

	// Upload the fonts for DearImgui
	VkCommandBuffer commandBuffer = beginSingleTimeCommands(uiCommandPool);
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	endSingleTimeCommands(commandBuffer, uiCommandPool);
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	// 在 ImGui 初始化时加载字体
	//ImGuiIO& io = ImGui::GetIO();
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\LBRITE.TTF", 13.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());
	//io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/calibril.ttf", 13.0, nullptr, io.Fonts->GetGlyphRangesDefault());

	//io.Fonts->AddFontFromFileTTF("C:/Windows/Fonts/arial.ttf", 13.0f, nullptr, io.Fonts->GetGlyphRangesChineseFull());

	//// 重要：重建字体纹理
	//io.Fonts->Build();
}

void Triangle::main_loop(void)
{
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		drawUI();

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
	vkResetCommandBuffer(uiCommandBuffers[currentFrame], 0);
	recordUICommands(uiCommandBuffers[currentFrame], imageIndex, currentFrame);

	std::vector<VkSemaphore> waitSemaphores{ imageAvailableSemaphores[currentFrame] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	std::array<VkCommandBuffer, 2> cmdBuffers = { commandBuffers[currentFrame], uiCommandBuffers[currentFrame] };
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
	cleanupUIResources();

	//part3:recreate swap chain
	swapChain_create();
	imageView_create();
	framebuffer_create();

	//imgui
	ImGui_ImplVulkan_SetMinImageCount(imageCount);
	createUICommandBuffers();
	createUIFramebuffers();
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

	//3.Swap Chain
	vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
}

void Triangle::clean_up(void)
{
	//vkFreeCommandBuffers(logicalDevice, commandPool, commandBuffers.size(), commandBuffers.data());
	cleanupSwapChain();

	// Cleanup DearImGui
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(logicalDevice, uiDescriptorPool, nullptr);
	vkFreeCommandBuffers(logicalDevice, uiCommandPool, static_cast<uint32_t>(uiCommandBuffers.size()),
		uiCommandBuffers.data());
	vkDestroyCommandPool(logicalDevice, uiCommandPool, nullptr);
	vkDestroyRenderPass(logicalDevice, uiRenderPass, nullptr);

	for (auto framebuffer : uiFramebuffers) {
		vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
	}

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
	{
		delete _sceneManager;
	}

	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
		
	vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
	vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
	vkDestroyRenderPass(logicalDevice, renderPass, nullptr);

	vkDestroyDevice(logicalDevice, nullptr);

	if (enableValidationLayers)
		DestoryDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	
	//要确保surface在instance之前被销毁
	vkDestroySurfaceKHR(instance, surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);
	glfwTerminate();
}


void Triangle::instance_create(void)
{
	if (enableValidationLayers && !CheckValidationLayerSupport()) {
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

	auto extensions = getRequiredExtensions();
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
}

// create debugMessenger
VkResult Triangle::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func == nullptr)
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
	return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
}

//destory debugMessenger
void Triangle::DestoryDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
		func(instance, debugMessenger, pAllocator);
}

void Triangle::debugMessenger_setUp(void)
{
	//如果不开启验证层，就不会又相应的验证信息反馈
	if (!enableValidationLayers) return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;

	if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to set up debug messenger!");
	}
}


void Triangle::surface_create(void)
{
	//用vulkan创建surface
	//配置相关信息
	
	VkWin32SurfaceCreateInfoKHR  surfaceCreateInfo{};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hwnd = glfwGetWin32Window(window);//获取窗口handle
	surfaceCreateInfo.hinstance = GetModuleHandle(nullptr);

	if (vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface))
	{
		throw std::runtime_error(" failed to create window surface .");
	}
	
	
	//if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
	//	throw std::runtime_error("failed to create window surface!");
	//}
	
}

QueueFamilyIndices Triangle::findQueueFamilies(VkPhysicalDevice devices)
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

bool Triangle::isDeviceSuitable(VkPhysicalDevice devices)
{
	//VkPhysicalDeviceProperties property;
	//VkPhysicalDeviceFeatures feature;

	//vkGetPhysicalDeviceProperties(devices, &property);
	//vkGetPhysicalDeviceFeatures(devices, &feature);

	// 独显 + geometryShader
	//return (property.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) && feature.geometryShader;

	QueueFamilyIndices indice = findQueueFamilies(devices);

	bool extensionSupport = CheckDeviceExtensionSupport(devices);//swapchain
	bool swapChainAdequate = false;

	if (extensionSupport)
	{
		SwapChainSupportDetails swapChainDetails = querySwapChainSupport(devices);
		swapChainAdequate = !(swapChainDetails.formats.empty() || swapChainDetails.presentModes.empty());
	}
	
	return indice.isComplete() && extensionSupport && swapChainAdequate;
}

void Triangle::physicalDevice_pick(void)
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
		if (isDeviceSuitable(phydevice))
		{
			physicalDevice = phydevice;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("no suitable device.");
	}

	//test code: tools
	PFN_vkGetPhysicalDeviceToolPropertiesEXT vkGetPhysicalDeviceToolPropertiesEXT = NULL;
	vkGetPhysicalDeviceToolPropertiesEXT = (PFN_vkGetPhysicalDeviceToolPropertiesEXT)vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceToolPropertiesEXT");
	if(vkGetPhysicalDeviceToolPropertiesEXT == NULL)
		throw std::runtime_error("get instance function failed");

	uint32_t toolCount = 0;
	auto result = vkGetPhysicalDeviceToolPropertiesEXT(physicalDevice, &toolCount, nullptr);
	if(toolCount <= 0 || result != VK_SUCCESS)
		throw std::runtime_error("tool of physical device is none");

	std::vector<VkPhysicalDeviceToolPropertiesEXT> tools(toolCount);
	result = vkGetPhysicalDeviceToolPropertiesEXT(physicalDevice, &toolCount, tools.data());
	if(result != VK_SUCCESS)
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

void Triangle::logicalDevice_create(void)
{
	QueueFamilyIndices indice = findQueueFamilies(physicalDevice);

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
	SwapChainSupportDetails swapChainDetails = querySwapChainSupport(physicalDevice);

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

	QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
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

VkShaderModule Triangle::createShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create shader module .");
	}

	return shaderModule;
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

	//only one subpass
	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0; //index of attachment
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;//作为图形管线
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0; //index of subpass
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &colorAttachment;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

void Triangle::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) 
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

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

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
}

void Triangle::graphicsPipline_create(void)
{
	auto vertexShaderCode = readFile(RELATIVE_PATH + std::string("/spvs/vert.spv"));
	auto fragShaderCode = readFile(RELATIVE_PATH + std::string("/spvs/frag.spv"));

	VkShaderModule vertShaderModule = createShaderModule(vertexShaderCode);
	VkShaderModule fragmentShaderModule = createShaderModule(fragShaderCode);

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

	//step 9:Pipeline layout 
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 0;
	pipelineLayoutInfo.pushConstantRangeCount = 0;

	         
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
	pipelineInfo.pDepthStencilState = nullptr;//optional
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

void Triangle::vertexBuffer_create(void)
{
	auto bufferSize = sizeof(vertices[0]) * vertices.size();
	vk::VulkanInit::createBuffer(physicalDevice, logicalDevice,  bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuffer, vertexBufferMem);

	//load data
	void* data;
	vkMapMemory(logicalDevice, vertexBufferMem, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), bufferSize);
	vkUnmapMemory(logicalDevice, vertexBufferMem);
}

void Triangle::framebuffer_create(void)
{
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (int i = 0; i < swapChainImageViews.size(); i++)
	{
		//specify the VkImageView objects that should be bound to the respective
		//attachment descriptions in the render pass pAttachment array
		VkImageView attachment[] = { swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
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
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

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

	VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	//The render pass commands will be embedded in the primary command buffer itself and no secondary command buffers will be executed.
	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

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

// get basic info of swap chain
//only query
SwapChainSupportDetails Triangle::querySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails swapChainDetails;

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


bool Triangle::CheckValidationLayerSupport(void)
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> layerProperties(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, layerProperties.data());

	std::cout << "all supported layer names" << "\n";
	for (const char* layerName : validationLayers)
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

bool Triangle::CheckDeviceExtensionSupport(VkPhysicalDevice devices)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(devices, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtension(extensionCount);
	vkEnumerateDeviceExtensionProperties(devices, nullptr, &extensionCount, availableExtension.data());

	std::set<std::string> deviceExtensions(requireExtensions.begin(), requireExtensions.end());

	for(const auto& extension : availableExtension)
	{
		deviceExtensions.erase(extension.extensionName);
	}

	//作为选择physical device的判定条件之一，不需要抛出程序错误，强制中断程序
	//if (!deviceExtensions.empty()) {
	//	throw std::runtime_error("extension not fulfill");
	//}

	return deviceExtensions.empty();
}

//glfw + layers information call back
std::vector<const char*> Triangle::getRequiredExtensions(void)
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
////************************Module Input**************************//
//void Triangle::vertexAndIndiceBuffer_create(std::vector<std::string> &paths)
//{
//	std::vector<obj::Vertex> objVertices;
//	std::vector<uint16_t> objIndices;
//
//	VkBuffer tempVertexBuffer = VK_NULL_HANDLE;
//	VkBuffer tempIndiceBuffer = VK_NULL_HANDLE;
//	VkDeviceMemory tempVertexBufferMem = VK_NULL_HANDLE;
//	VkDeviceMemory tempIndiceBufferMem = VK_NULL_HANDLE;
//
//	if (vertexBuffer != VK_NULL_HANDLE)
//	{
//		tempVertexBuffer = vertexBuffer;
//		tempIndiceBuffer = indiceBuffer;
//		tempVertexBufferMem = vertexBufferMem;
//		tempIndiceBufferMem = indiceBufferMem;
//	}
//
//	for (int i = 0; i < paths.size(); ++i)
//	{
//		Object* ob = new Object(paths[i].c_str());
//		auto verticesIn = ob->getVertices();
//		auto indices = ob->getIndices();
//
//		objVertices.insert(objVertices.end(), verticesIn.begin(), verticesIn.end());
//		objIndices.insert(objIndices.end(), indices.begin(), indices.end());
//
//		delete ob;
//	}
//
//	if (objVertices.size() == 0 || objIndices.size() == 0)
//	{
//		std::runtime_error("not have vertices or indices data");
//		return;
//	}
//
//	modelIndicesNum = objIndices.size();
//	//Vertex buffer
//	auto bufferSize = sizeof(objVertices[0]) * objVertices.size();
//	createBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, vertexBuffer, vertexBufferMem);
//
//	//load data
//	void* data;
//	vkMapMemory(logicalDevice, vertexBufferMem, 0, bufferSize, 0, &data);
//	memcpy(data, objVertices.data(), bufferSize);
//	vkUnmapMemory(logicalDevice, vertexBufferMem);
//
//	bufferSize = sizeof(uint16_t) * objIndices.size();
//	createBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
//		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, indiceBuffer, indiceBufferMem);
//
//	vkMapMemory(logicalDevice, indiceBufferMem, 0, bufferSize, 0, &data);
//	memcpy(data, objIndices.data(), bufferSize);
//	vkUnmapMemory(logicalDevice, indiceBufferMem);
//
//	if (tempVertexBuffer != VK_NULL_HANDLE)
//	{
//		vkDestroyBuffer(logicalDevice, tempVertexBuffer, nullptr);
//		vkDestroyBuffer(logicalDevice, tempIndiceBuffer, nullptr);
//		vkFreeMemory(logicalDevice, tempVertexBufferMem, nullptr);
//		vkFreeMemory(logicalDevice, tempIndiceBufferMem, nullptr);
//	}
//
//	return;
//}

//************************IIMGUI********************************//
void Triangle::createUICommandBuffers() {
	uiCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = uiCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(uiCommandBuffers.size());

	if (vkAllocateCommandBuffers(logicalDevice, &commandBufferAllocateInfo, uiCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Unable to allocate UI command buffers!");
	}
}

void Triangle::recordUICommands(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame) {
	VkCommandBufferBeginInfo cmdBufferBegin = {};
	cmdBufferBegin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufferBegin.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(commandBuffer, &cmdBufferBegin) != VK_SUCCESS) {
		throw std::runtime_error("Unable to start recording UI command buffer!");
	}

	VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = uiRenderPass;
	renderPassBeginInfo.framebuffer = uiFramebuffers[imageIndex];
	renderPassBeginInfo.renderArea.extent.width = swapChainExtent.width;
	renderPassBeginInfo.renderArea.extent.height = swapChainExtent.height;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	memoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	vkCmdWaitEvents(commandBuffer, 1, &event[curFrame], srcStageMask, dstStageMask, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

	// Grab and record the draw data for Dear Imgui
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	// End and submit render pass
	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffers!");
	}
}

void Triangle::createUICommandPool(VkCommandPool* cmdPool, VkCommandPoolCreateFlags flags) {

	QueueFamilyIndices indice = findQueueFamilies(physicalDevice);

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = indice.graphicsFamily.value();
	commandPoolCreateInfo.flags = flags;

	if (vkCreateCommandPool(logicalDevice, &commandPoolCreateInfo, nullptr, cmdPool) != VK_SUCCESS) {
		throw std::runtime_error("Could not create graphics command pool!");
	}
}

// Copied this code from DearImgui's setup:
// https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp
void Triangle::createUIDescriptorPool() {
	VkDescriptorPoolSize pool_sizes[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info = {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
	pool_info.poolSizeCount = static_cast<uint32_t>(IM_ARRAYSIZE(pool_sizes));
	pool_info.pPoolSizes = pool_sizes;
	if (vkCreateDescriptorPool(logicalDevice, &pool_info, nullptr, &uiDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Cannot allocate UI descriptor pool!");
	}
}

void Triangle::createUIFramebuffers() {
	// Create some UI framebuffers. These will be used in the render pass for the UI
	uiFramebuffers.resize(swapChainImages.size());
	VkImageView attachment[1];
	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = uiRenderPass;
	info.attachmentCount = 1;
	info.pAttachments = attachment;
	info.width = swapChainExtent.width;
	info.height = swapChainExtent.height;
	info.layers = 1;
	for (uint32_t i = 0; i < swapChainImages.size(); ++i) {
		attachment[0] = swapChainImageViews[i];
		if (vkCreateFramebuffer(logicalDevice, &info, nullptr, &uiFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create UI framebuffers!");
		}
	}
}

void Triangle::createUIRenderPass() {
	// Create an attachment description for the render pass
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.format = swapChainImageFormat;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Need UI to be drawn on top of main
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Last pass so we want to present after
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	// Create a color attachment reference
	VkAttachmentReference attachmentReference = {};
	attachmentReference.attachment = 0;
	attachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Create a subpass
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &attachmentReference;

	// Create a subpass dependency to synchronize our main and UI render passes
	// We want to render the UI after the geometry has been written to the framebuffer
	// so we need to configure a subpass dependency as such
	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL; // Create external dependency
	subpassDependency.dstSubpass = 0; // The geometry subpass comes first
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // Wait on writes
	subpassDependency.dstStageMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	// Finally create the UI render pass
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.pAttachments = &attachmentDescription;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &subpassDependency;

	if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &uiRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create UI render pass!");
	}
}

VkCommandBuffer Triangle::beginSingleTimeCommands(VkCommandPool cmdPool) {
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = cmdPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer = {};
	vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("Could not create one-time command buffer!");
	}

	return commandBuffer;
}

void Triangle::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool cmdPool) {
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(logicalDevice, cmdPool, 1, &commandBuffer);
}

void Triangle::cleanupUIResources(void)
{
	for (auto framebuffer : uiFramebuffers) {
		vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(logicalDevice, uiCommandPool,
		static_cast<uint32_t>(uiCommandBuffers.size()), uiCommandBuffers.data());
}

void Triangle::drawUI() {
	// Start the Dear ImGui frame
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	static float f = 0.0f;
	static int counter = 0;

	ImGui::Begin("Renderer Options");
	ImGui::Text("This is some useful text.");
	ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
	if (ImGui::Button("Button")) {
		counter++;
	}
	ImGui::SameLine();
	ImGui::Text("counter = %d", counter);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	//show file info

	for (int i = 0; i < filePaths.size(); ++i)
	{
		ImGui::Text("%s", filePaths[i].c_str());

	}

	//ImOGuizmo::SetRect(10.0f /* x */, 600 /* y */, 120.0f /* square size */);
	//ImOGuizmo::BeginFrame();

	//auto viewMat = glm::lookAt(glm::vec3(1, 1, 0), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0));
	//auto projMat = glm::perspective(static_cast<float>(glm::radians(60.0)), 0.5f, 0.01f, 100.0f);
	//ImOGuizmo::DrawGizmo(glm::value_ptr(viewMat), glm::value_ptr(projMat), 0.0 /* optional: default = 0.0f */);

	ImGui::End();

	ImGui::Render();
}

