#pragma once
#include "vulkanInit.h"
#include "base.h"
#include "camera.h"

#include <iostream>
#include <vector>

namespace scene {

// Single vertex buffer for all primitives
struct VerticeResource {
	//int count = 0;
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
	std::vector<base::Meshlet> meshlets;
};

//struct BaseModel : public Model
//{
//	VerticeResource vertex;
//	IndiceResource indice;
//};
//
//struct MeshModel : public Model
//{
//	std::vector<base::Meshlet> meshlets;
//};

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
	
	//camera
	glm::mat4 getModelMatrix(void);
	glm::mat4 getProjMatrix(int width, int height);
	glm::mat4 getViewMatrix(void);

	void setModelMatrix(glm::vec3& rotate, glm::vec3& translate);

private:

	std::vector<Model> _models;
	scene::Camera* _camera = nullptr;

	// The class requires some Vulkan objects so it can create it's own resources
	VkPhysicalDevice _physicalDevice;
	VkDevice _logicalDevice;
	VkQueue _queue;

	void vertexAndIndiceBuffer_create(Model &model, std::vector<base::Vertex> &vertices, std::vector<uint32_t> &indices);
	void paskAsMeshlets(std::vector<base::Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<base::Meshlet>& meshlets);
};
}