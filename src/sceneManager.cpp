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
	_camera = new Camera();

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

	delete _camera;
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
	//model.vertex.count = vertices.size();
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

/*******************Camera********************/
glm::mat4 SceneManager::getModelMatrix()
{
	assert(_camera != nullptr);
	return _camera->getModelMatrix();
}

glm::mat4 SceneManager::getProjMatrix(int width, int height)
{
	assert(_camera != nullptr);
	return _camera->getProjMatrix(width, height);
}

glm::mat4 SceneManager::getViewMatrix()
{
	assert(_camera != nullptr);
	return _camera->getViewMatrix();
}

void SceneManager::setModelMatrix(glm::vec3& rotate, glm::vec3& translate)
{
	assert(_camera != nullptr);
	_camera->setModelMatrix(rotate, translate);
}

/*****************MESH SHADER*******************/
void SceneManager::paskAsMeshlets(std::vector<base::Vertex>& vertices, std::vector<uint32_t>& indices, std::vector<base::Meshlet> &meshlets)
{
	//size_t vertexBufferSize = vertexBuffer.size();
	size_t indexBufferSize = indices.size();

	struct StagingBuffer {
		VkBuffer buffer;
		VkDeviceMemory memory;
	} vertexStaging, indexStaging;

	base::Meshlet meshlet = {};

	std::vector<uint8_t> meshletVertices(vertices.size(), 0xff);

	bool flag = false;
	for (size_t i = 0; i < indexBufferSize; i += 3)
	{
		unsigned int a = indices[i + 0];
		unsigned int b = indices[i + 1];
		unsigned int c = indices[i + 2];

		uint8_t& av = meshletVertices[a];
		uint8_t& bv = meshletVertices[b];
		uint8_t& cv = meshletVertices[c];

		if (meshlet.vertexCount + (av == 0xff) + (bv == 0xff) + (cv == 0xff) > 64 || meshlet.indexCount + 3 > 126 * 3)
		{

			meshlets.emplace_back(meshlet);
			flag = true;

			for (size_t j = 0; j < meshlet.vertexCount; ++j)
				meshletVertices[meshlet.vertices[j]] = 0xff;

			meshlet = {};
			//}
			//else
			//	break;
		}

		if (av == 0xff)
		{
			av = meshlet.vertexCount;
			meshlet.vertices[meshlet.vertexCount++] = a;
		}

		if (bv == 0xff)
		{
			bv = meshlet.vertexCount;
			meshlet.vertices[meshlet.vertexCount++] = b;
		}

		if (cv == 0xff)
		{
			cv = meshlet.vertexCount;
			meshlet.vertices[meshlet.vertexCount++] = c;
		}

		meshlet.indices[meshlet.indexCount++] = av;
		meshlet.indices[meshlet.indexCount++] = bv;
		meshlet.indices[meshlet.indexCount++] = cv;
	}

	if (meshlet.indexCount)
		meshlets.emplace_back(meshlet);
}

//void SceneManager::createMeshStorageBuffer(std::vector<meshShader::Vertex>& vertices, std::vector<meshShader::Meshlet>& meshlets)
//{
//	verticeStorageBufferSize = vertices.size() * sizeof(meshShader::Vertex);
//	meshletStorageBufferSize = meshlets.size() * sizeof(meshShader::Meshlet);
//
//	VkBuffer stagingBuffer;
//	VkDeviceMemory stagingBufferMem;
//
//	//vertices
//	createBuffer(verticeStorageBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//		stagingBuffer, stagingBufferMem);
//
//	void* data;
//	vkMapMemory(logicalDevice, stagingBufferMem, 0, verticeStorageBufferSize, 0, &data);
//	memcpy(data, vertices.data(), (size_t)verticeStorageBufferSize);
//	vkUnmapMemory(logicalDevice, stagingBufferMem);
//
//	createBuffer(verticeStorageBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, verticeStorageBuffer, verticeStorageBufferMem);
//
//	copyBuffer(stagingBuffer, verticeStorageBuffer, verticeStorageBufferSize);
//
//	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
//	vkFreeMemory(logicalDevice, stagingBufferMem, nullptr);
//
//	//meshlet
//	createBuffer(meshletStorageBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//		stagingBuffer, stagingBufferMem);
//
//	vkMapMemory(logicalDevice, stagingBufferMem, 0, meshletStorageBufferSize, 0, &data);
//	memcpy(data, meshlets.data(), (size_t)meshletStorageBufferSize);
//	vkUnmapMemory(logicalDevice, stagingBufferMem);
//
//	createBuffer(meshletStorageBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
//		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, meshletStorageBuffer, meshletStorageBufferMem);
//
//	copyBuffer(stagingBuffer, meshletStorageBuffer, meshletStorageBufferSize);
//
//	vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
//	vkFreeMemory(logicalDevice, stagingBufferMem, nullptr);
//}