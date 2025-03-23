#include "ui.h"
#include <iostream>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

#define IMOGUIZMO_LEFT_HANDED
#include "Gizmo/imoguizmo.hpp"

//************************IIMGUI********************************//
UI::UI(VkPhysicalDevice &pdev, VkDevice &dev, VkSurfaceKHR &surface, VkFormat format, VkExtent2D extent): _pdev(pdev), _dev(dev), _surface(surface), _format(format), _extent(extent)
{

}

UI::~UI()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	vkDestroyDescriptorPool(_dev, uiDescriptorPool, nullptr);
	vkFreeCommandBuffers(_dev, uiCommandPool, static_cast<uint32_t>(uiCommandBuffers.size()),
		uiCommandBuffers.data());
	vkDestroyCommandPool(_dev, uiCommandPool, nullptr);
	vkDestroyRenderPass(_dev, uiRenderPass, nullptr);

	for (auto framebuffer : uiFramebuffers) {
		vkDestroyFramebuffer(_dev, framebuffer, nullptr);
	}
}

void UI::init(GLFWwindow* window, VkInstance &instance, VkQueue &queue, std::vector<VkImageView>& imageViews)
{
	_imageCount = imageViews.size();
	_paraCount = MAX_FRAMES_IN_FLIGHT;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Initialize some DearImgui specific resources
	createUIDescriptorPool();
	createUIRenderPass();
	createUICommandPool(&uiCommandPool, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	createUICommandBuffers();
	createUIFramebuffers(imageViews);

	vk::QueueFamilyIndices indice = vk::VulkanInit::findQueueFamilies(_pdev, _surface);

	// Provide bind points from Vulkan API
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = _pdev;
	init_info.Device = _dev;
	init_info.QueueFamily = indice.graphicsFamily.value();
	init_info.Queue = queue;
	init_info.DescriptorPool = uiDescriptorPool;
	init_info.MinImageCount = _imageCount;
	init_info.ImageCount = _imageCount;
	ImGui_ImplVulkan_Init(&init_info, uiRenderPass);

	// Upload the fonts for DearImgui
	VkCommandBuffer commandBuffer = vk::VulkanInit::beginSingleTimeCommands(_dev, uiCommandPool);
	ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
	vk::VulkanInit::endSingleTimeCommands(_dev, queue, commandBuffer, uiCommandPool);
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

void UI::recreateUISwapChain(uint32_t count, std::vector<VkImageView>& imageViews)
{
	ImGui_ImplVulkan_SetMinImageCount(count);
	createUICommandBuffers();
	createUIFramebuffers(imageViews);
}

void UI::recordUICommandBuffer(VkEvent &event, uint32_t imageIndex, uint32_t curFrame)
{
	vkResetCommandBuffer(uiCommandBuffers[curFrame], 0);
	recordUICommands(uiCommandBuffers[curFrame], event, imageIndex, curFrame);
}

void UI::createUICommandBuffers() {
	uiCommandBuffers.resize(_paraCount);

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = uiCommandPool;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandBufferCount = static_cast<uint32_t>(uiCommandBuffers.size());

	if (vkAllocateCommandBuffers(_dev, &commandBufferAllocateInfo, uiCommandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("Unable to allocate UI command buffers!");
	}
}

void UI::cleanupUIResources(void)
{
	for (auto framebuffer : uiFramebuffers) {
		vkDestroyFramebuffer(_dev, framebuffer, nullptr);
	}

	vkFreeCommandBuffers(_dev, uiCommandPool,
		static_cast<uint32_t>(uiCommandBuffers.size()), uiCommandBuffers.data());
}

void UI::draw(base::UniformBufferObject& ubo) {
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

	//for (int i = 0; i < filePaths.size(); ++i)
	//{
	//	ImGui::Text("%s", filePaths[i].c_str());

	//}

	ImOGuizmo::SetRect(50.0f /* x */, HEIGHT - 130.0f /* y */, 80.0f /* square size */);
	ImOGuizmo::BeginFrame();

	auto matrix = ubo.view * ubo.model;
	ImOGuizmo::DrawGizmo(glm::value_ptr(matrix), glm::value_ptr(ubo.projection), 0.0 /* optional: default = 0.0f */);

	//auto viewMat = glm::lookAt(glm::vec3(1, 1, 0), glm::vec3(0, 1, 0), glm::vec3(1, 0, 0));
	//auto projMat = glm::perspective(static_cast<float>(glm::radians(60.0)), 0.5f, 0.01f, 100.0f);

	ImGui::End();

	ImGui::Render();
}

void UI::recordUICommands(VkCommandBuffer &commandBuffer, VkEvent &event, uint32_t imageIndex, uint32_t curFrame) {
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
	renderPassBeginInfo.renderArea.extent.width = _extent.width;
	renderPassBeginInfo.renderArea.extent.height = _extent.height;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkMemoryBarrier memoryBarrier = {};
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	memoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	vkCmdWaitEvents(commandBuffer, 1, &event, srcStageMask, dstStageMask, 1, &memoryBarrier, 0, nullptr, 0, nullptr);

	// Grab and record the draw data for Dear Imgui
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

	// End and submit render pass
	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to record command buffers!");
	}
}

void UI::createUICommandPool(VkCommandPool* cmdPool, VkCommandPoolCreateFlags flags) {

	vk::QueueFamilyIndices indice = vk::VulkanInit::findQueueFamilies(_pdev, _surface);

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.queueFamilyIndex = indice.graphicsFamily.value();
	commandPoolCreateInfo.flags = flags;

	if (vkCreateCommandPool(_dev, &commandPoolCreateInfo, nullptr, cmdPool) != VK_SUCCESS) {
		throw std::runtime_error("Could not create graphics command pool!");
	}
}

// Copied this code from DearImgui's setup:
// https://github.com/ocornut/imgui/blob/master/examples/example_glfw_vulkan/main.cpp
void UI::createUIDescriptorPool() {
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
	if (vkCreateDescriptorPool(_dev, &pool_info, nullptr, &uiDescriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("Cannot allocate UI descriptor pool!");
	}
}

void UI::createUIFramebuffers(std::vector<VkImageView> &imageViews) {
	// Create some UI framebuffers. These will be used in the render pass for the UI
	uiFramebuffers.resize(imageViews.size());
	VkImageView attachment[1];
	VkFramebufferCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	info.renderPass = uiRenderPass;
	info.attachmentCount = 1;
	info.pAttachments = attachment;
	info.width = _extent.width;
	info.height = _extent.height;
	info.layers = 1;

	for (uint32_t i = 0; i < imageViews.size(); ++i) {
		attachment[0] = imageViews[i];
		if (vkCreateFramebuffer(_dev, &info, nullptr, &uiFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("Unable to create UI framebuffers!");
		}
	}
}

void UI::createUIRenderPass() {
	// Create an attachment description for the render pass
	VkAttachmentDescription attachmentDescription = {};
	attachmentDescription.format = _format;
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

	if (vkCreateRenderPass(_dev, &renderPassInfo, nullptr, &uiRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("Unable to create UI render pass!");
	}
}