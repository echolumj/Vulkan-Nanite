#pragma once
#include "vulkanInit.h"
#include "base.h"

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

class UI {
public:
	UI(VkPhysicalDevice &pdev, VkDevice& dev, VkSurfaceKHR& surface, VkFormat format, VkExtent2D extent);
	~UI();

	void init(GLFWwindow* window, VkInstance& instance, VkQueue& queue, std::vector<VkImageView>& imageViews);
	void recreateUISwapChain(uint32_t count, std::vector<VkImageView>& imageViews);
	void recordUICommandBuffer(VkEvent& event, uint32_t imageIndex, uint32_t curFrame);
	void cleanupUIResources(void);
	void draw(base::UniformBufferObject &ubo);

	VkCommandBuffer getCommandBuffer(uint32_t curFrame) { return uiCommandBuffers[curFrame]; }

private:
	void createUICommandBuffers(void);
	void recordUICommands(VkCommandBuffer &commandBuffer, VkEvent &event, uint32_t imageIndex, uint32_t curFrame);
	void createUICommandPool(VkCommandPool* cmdPool, VkCommandPoolCreateFlags flags);
	void createUIDescriptorPool(void);
	void createUIFramebuffers(std::vector<VkImageView>& imageViews);
	void createUIRenderPass(void);

	//imgui UI
	VkDescriptorPool uiDescriptorPool;
	std::vector<VkFramebuffer> uiFramebuffers;
	VkRenderPass uiRenderPass;
	VkCommandPool uiCommandPool;
	std::vector<VkCommandBuffer> uiCommandBuffers;

	VkFormat _format;
	VkExtent2D _extent;
	uint32_t _imageCount;
	uint8_t _paraCount;

	VkDevice _dev;
	VkPhysicalDevice _pdev;
	VkSurfaceKHR _surface;
};