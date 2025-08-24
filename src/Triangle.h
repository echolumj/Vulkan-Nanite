#pragma once

#define GLFW_INCLUDE_VULKAN
#define GLFW_EXPOSE_NATIVE_WIN32
//#include <Windows.h>

#include "sceneManager.h"
#include "ui.h"
#include <vulkan/vulkan_win32.h>
#include <array>


namespace triangle
{
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 color;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
			std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3, VkVertexInputAttributeDescription{});

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, normal);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}
	};

}
using namespace triangle;

class Triangle
{
public:
	Triangle();
	void run(void);

	//确保window size一旦发生改变，就触发相应的error
	bool framebufferResized;

private:
	void window_init(void);
	void vulkan_init(void);
	void main_loop(void);
	void clean_up(void);
	void drawFrame(void); 
	void graphicsPipline_create(void);
	void framebuffer_create(void);

	void syncObjects_create(void);
	void swapCahin_recreate(void);
	void depthResources_create(void);

	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t curFrame);
	void cleanupSwapChain(void);
	void sceneInteract(void);

	VkFormat findDepthFormat(void);
	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);


	//handle
	GLFWwindow* window;
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;

	vk::SwapChainInfo swapChainInfo= {};
	VkSwapchainKHR swapChain;

	VkImage depthImage;
	VkImageView depthImageView;
	VkDeviceMemory depthImageMemory;
	VkFormat depthImageFormat;

	VkPipelineLayout pipelineLayout;;
	VkRenderPass renderPass;
	VkPipeline graphicsPipeline;
	std::vector<VkFramebuffer> swapChainFramebuffers;

	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	VkQueue graphicsQueue;//queue handle ,clean up implicitly
	VkQueue presentQueue;
	VkQueue computeQueue;

	//GPU-GPU Synchronization
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	//CPU-GPU Synchronization
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight; //image count
	std::vector <VkEvent> event;

	size_t currentFrame;
	uint32_t imageCount;

	scene::SceneManager* _sceneManager;
	UI* _ui;
	
	uint16_t curModelId = -1;
	base::UniformBufferObject ubo;
};

