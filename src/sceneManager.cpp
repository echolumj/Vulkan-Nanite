#include "sceneManager.h"
#include "object.h"
#include "gltf.h"

using namespace scene;

/****************Model*******************/
//Model::Model(std::string name)
//{
//	//Determine the model type
//
//	//Create a model of the corresponding type
//
//}
//
//Model::~Model()
//{
//	//delete the corresponding resource
//
//}


/****************SceneManager*******************/
SceneManager::SceneManager(VkPhysicalDevice physicalDevice, VkDevice &logicDevice):_physicalDevice(physicalDevice), _logicalDevice(logicDevice)
{
	//support 

}

SceneManager::~SceneManager()
{
	for (uint16_t id = 0; id < _models.size(); ++id)
	{
		auto model = _models[id];
		vkDestroyBuffer(_logicalDevice, model.vertex.buffer, nullptr);
		vkFreeMemory(_logicalDevice, model.vertex.memory, nullptr);
		vkDestroyBuffer(_logicalDevice, model.indice.buffer, nullptr);
		vkFreeMemory(_logicalDevice, model.indice.memory, nullptr);
	}
}

uint16_t SceneManager::addModel(std::string name)
{
	//file type
	//file type
	std::string fileType = name.substr(name.rfind('.'), name.length());

	if ( fileType.compare(".obj") != 0 && fileType.compare(".gltf") != 0)
		return false;
	
	Model  model;

	if (fileType.compare(".obj") == 0)
	{
		std::vector<uint32_t> indices;
		std::vector<base::Vertex> vertices;

		obj::Object *objFile = new obj::Object(name);
		indices = objFile->getIndices();
		vertices = objFile->getVertices();
		delete objFile;

		vertexAndIndiceBuffer_create(model, vertices, indices);
		_models.push_back(model);

		return _models.size()-1;
	}
	else if(fileType.compare(".gltf") == 0)
	{
		std::vector<uint32_t> indices;
		std::vector<base::Vertex> vertices;
		gltf::GLTFModel* gltfFile = new gltf::GLTFModel();
		gltfFile->loadglTFFile(name,  indices, vertices);

		vertexAndIndiceBuffer_create(model, vertices, indices);
		_models.push_back(model);

		return _models.size() - 1;
	}

	return -1;

}

Model SceneManager::getModel(uint16_t id) const
{
	assert(id < _models.size());

	return _models[id];
}

//************************Module Input**************************//
void SceneManager::vertexAndIndiceBuffer_create(Model &model, std::vector<base::Vertex> &vertices, std::vector<uint32_t> &indices)
{
	if (vertices.size() == 0 || indices.size() == 0)
	{
		std::runtime_error("not have vertices or indices data");
		return;
	}

	//Vertex buffer
	auto bufferSize = sizeof(vertices[0]) * vertices.size();
	vk::VulkanInit::createBuffer(_physicalDevice, _logicalDevice, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, model.vertex.buffer, model.vertex.memory);

	//load data
	void* data;
	vkMapMemory(_logicalDevice, model.vertex.memory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), bufferSize);
	vkUnmapMemory(_logicalDevice, model.vertex.memory);

	model.indice.count = indices.size();
	bufferSize = sizeof(indices[0]) * indices.size();
	vk::VulkanInit::createBuffer(_physicalDevice, _logicalDevice, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, model.indice.buffer, model.indice.memory);

	vkMapMemory(_logicalDevice, model.indice.memory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), bufferSize);
	vkUnmapMemory(_logicalDevice, model.indice.memory);

	return;
}
