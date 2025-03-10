#pragma once
#include "vulkanInit.h"

#include <iostream>
#include <vector>

#include "object.h"

namespace scene {

// Single vertex buffer for all primitives
struct VerticeResource {
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

// Single index buffer for all primitives
struct IndiceResource {
	int count = 0;
	VkBuffer buffer = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct  Model
{
	VerticeResource vertex;
	IndiceResource indice;
};

//class Model {
//public:
//	Model(std::string name);
//	~Model();
//
//private:
//	VerticeResource* _vertexRes = nullptr;
//	IndiceResource* _indiceRes = nullptr;
//};


class SceneManager {
public:
	SceneManager(VkPhysicalDevice physicalDevice, VkDevice& logicDevice);
	~SceneManager();

	uint16_t addModel(std::string name);
	Model getModel(uint16_t id) const;
	uint16_t getModelSize(void) { return _models.size(); }


private:

	std::vector<Model> _models;

	// The class requires some Vulkan objects so it can create it's own resources
	VkPhysicalDevice _physicalDevice;
	VkDevice _logicalDevice;
	VkQueue _queue;

	void vertexAndIndiceBuffer_create(Model &model, std::vector<obj::Vertex> &vertices, std::vector<uint32_t> &indices);
};
}