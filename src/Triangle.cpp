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
	_ui = new UI(physicalDevice, logicalDevice, surface, swapChainInfo.format, swapChainInfo.extent);
	_ui->init(window, instance, graphicsQueue, swapChainInfo.imageViews);

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
	vk::VulkanInit::createLogicalDevice(logicalDevice, physicalDevice, surface, requireExtensions, enableValidationLayers);

	//get queues
	vk::QueueFamilyIndices indice = vk::VulkanInit::findQueueFamilies(physicalDevice, surface);
	vkGetDeviceQueue(logicalDevice, indice.graphicAndComputeFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, indice.graphicAndComputeFamily.value(), 0, &computeQueue);
	vkGetDeviceQueue(logicalDevice, indice.presentFamily.value(), 0, &presentQueue);


	vk::VulkanInit::createSwapChain(swapChainInfo, swapChain, physicalDevice, logicalDevice, surface, window);
	vk::QueueFamilyIndices queueFamilyIndices = vk::VulkanInit::findQueueFamilies(physicalDevice, surface);
	vk::VulkanInit::createCommandPool(commandPool, logicalDevice, queueFamilyIndices.graphicsFamily.value());
	commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	vk::VulkanInit::createCommondBuffers(commandBuffers, logicalDevice, commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	depthResources_create();

	//only one color buffer
	VkAttachmentDescription colorAttachment = vk::VulkanInit::AttachmentDescription(swapChainInfo.format, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkAttachmentDescription depthAttachment = vk::VulkanInit::AttachmentDescription(depthImageFormat, VK_SAMPLE_COUNT_1_BIT, VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	std::vector<VkAttachmentDescription> colorAttachments = { colorAttachment };
	VkSubpassDescription subpass = vk::VulkanInit::Subpass(colorAttachments, depthAttachment);
	VkSubpassDependency dependency = vk::VulkanInit::SubpassDependency(VK_SUBPASS_EXTERNAL, 0, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
	std::vector<VkSubpassDescription> subpasses = { subpass };
	std::vector<VkSubpassDependency> dependencies = { dependency };
	vk::VulkanInit::createRenderPass(renderPass, logicalDevice, colorAttachments, depthAttachment, subpasses, dependencies);

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
	vk::VulkanInit::createSwapChain(swapChainInfo, swapChain, physicalDevice, logicalDevice, surface, window);
	depthResources_create();
	framebuffer_create();

	//UI
	_ui->cleanupUIResources();
	_ui->recreateUISwapChain(imageCount, swapChainInfo.imageViews);
}

void Triangle::cleanupSwapChain(void)
{
	//1.FrameBuffer 
	for (auto framebuffer : swapChainFramebuffers)
	{
		vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
	}

	//2.Swap Chain Images View
	for (auto imageView : swapChainInfo.imageViews)
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


void Triangle::graphicsPipline_create(void)
{
	const std::string glslcPath = std::string("D:/0_Software/VulkanSDK/1.3.290.0/Bin/glslc.exe");

	bool fragResult = vk::VulkanInit::CompileShader(glslcPath, std::string(SHADER_DIR)+ std::string("/shader.frag"), std::string(SPV_DIR) + std::string("/frag.spv"));
	bool vertResult = vk::VulkanInit::CompileShader(glslcPath, std::string(SHADER_DIR) + std::string("/shader.vert"), std::string(SPV_DIR) + std::string("/vert.spv"));

	assert(fragResult && vertResult);

	auto vertexShaderCode = readFile(RELATIVE_PATH + std::string("/spvs/vert.spv"));
	auto fragShaderCode = readFile(RELATIVE_PATH + std::string("/spvs/frag.spv"));

	VkShaderModule vertShaderModule = vk::VulkanInit::createShaderModule(logicalDevice, vertexShaderCode);
	VkShaderModule fragmentShaderModule = vk::VulkanInit::createShaderModule(logicalDevice, fragShaderCode);

	//step 1:prepare Shader Stage
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = vk::VulkanInit::PipelineShaderStage(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule);
	VkPipelineShaderStageCreateInfo fragShaderStageInfo = vk::VulkanInit::PipelineShaderStage(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule);
	VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
	
	//step 2:prepare Vertex Input State
	auto bindingDesc = Vertex::getBindingDescription();
	auto attributeDesc = Vertex::getAttributeDescriptions();
	std::vector<VkVertexInputBindingDescription> bindingDescs = { bindingDesc };
	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = vk::VulkanInit::PipelineVertexInputState(bindingDescs, attributeDesc);
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = vk::VulkanInit::PipelineInputAssemblyState(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	//Viewports and scissors
	std::vector<VkViewport> viewports = { VkViewport{0.0f, 0.0f, (float)swapChainInfo.extent.width , (float)swapChainInfo.extent.height, 0.0f, 1.0f } };
	std::vector<VkRect2D> scissors = { VkRect2D{ VkOffset2D{ 0, 0 } , swapChainInfo.extent } };
	VkPipelineViewportStateCreateInfo viewportCreateInfo = vk::VulkanInit::PipelineViewportState(viewports, scissors);
	VkPipelineRasterizationStateCreateInfo rasterCreateInfo = vk::VulkanInit::PipelineRasterizationState(VK_FALSE, VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = vk::VulkanInit::PipelineMultisampleState(VK_FALSE, VK_SAMPLE_COUNT_1_BIT);
	VkPipelineDepthStencilStateCreateInfo depth_info = vk::VulkanInit::PipelineDepthStencilState(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS, VK_FALSE, VK_FALSE);
	VkPipelineColorBlendAttachmentState colorBlendAttachment = vk::VulkanInit::PipelineColorBlendAttachmentState(VK_FALSE, VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

	std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments = { colorBlendAttachment };
	VkPipelineColorBlendStateCreateInfo colorBlendState = vk::VulkanInit::PipelineColorBlendState(VK_FALSE, colorBlendAttachments);
	std::vector <VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
	VkPipelineDynamicStateCreateInfo dynamicState = vk::VulkanInit::PipelineDynamicState(dynamicStates);
	VkPushConstantRange pushConstant = vk::VulkanInit::PushConstantRange(0, sizeof(base::UniformBufferObject), VK_SHADER_STAGE_VERTEX_BIT);

	std::vector<VkPushConstantRange> pushConstants = { pushConstant };
	vk::VulkanInit::createPipelineLayout(pipelineLayout, logicalDevice, pushConstants);

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
	swapChainFramebuffers.resize(swapChainInfo.imageViews.size());

	for (int i = 0; i < swapChainInfo.imageViews.size(); i++)
	{
		std::vector<VkImageView> attachment = { swapChainInfo.imageViews[i] };
		vk::VulkanInit::createFrameBuffer(swapChainFramebuffers[i], logicalDevice, renderPass, attachment, depthImageView, swapChainInfo.extent);
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
	renderPassInfo.renderArea.extent = swapChainInfo.extent;

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
	viewport.width = static_cast<float>(swapChainInfo.extent.width);
	viewport.height = static_cast<float>(swapChainInfo.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChainInfo.extent;
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


void Triangle::syncObjects_create(void)
{
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	event.resize(MAX_FRAMES_IN_FLIGHT);

	imagesInFlight.resize(swapChainInfo.images.size(), VK_NULL_HANDLE);

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
	vk::VulkanInit::createImage(physicalDevice, logicalDevice, swapChainInfo.extent.width, swapChainInfo.extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
	vk::VulkanInit::createImageView(logicalDevice, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, depthImageView);
	vk::VulkanInit::transitionImageLayout(logicalDevice, graphicsQueue, commandPool, depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
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
		rotationAngles.x = static_cast<float>(-deltaY) * 0.01f; // 绕X轴旋转
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
