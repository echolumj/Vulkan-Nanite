#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tinygltf/tiny_gltf.h>

#include <iostream>

namespace gltf {

// The vertex layout for the samples' model
struct Vertex {
	glm::vec4 pos;
	glm::vec3 normal;
	//glm::vec2 uv;
	glm::vec3 color;
};

class GLTFModel
{
public:


	// The following structures roughly represent the glTF scene structure
	// To keep things simple, they only contain those properties that are required for this sample
	struct Node;

	// A primitive contains the data for a single draw call
	struct Primitive {
		uint32_t firstIndex;
		uint32_t indexCount;
		int32_t materialIndex;
	};

	// Contains the node's (optional) geometry and can be made up of an arbitrary number of primitives
	struct Mesh {
		std::vector<Primitive> primitives;
	};

	// A node represents an object in the glTF scene graph
	struct Node {
		Node* parent;
		std::vector<Node*> children;
		Mesh mesh;
		glm::mat4 matrix;
		~Node() {
			for (auto& child : children) {
				delete child;
			}
		}
	};

	// A glTF material stores information in e.g. the texture that is attached to it and colors
	struct Material {
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		uint32_t baseColorTextureIndex;
	};

	// Contains the texture for a single glTF image
	// Images may be reused by texture objects and are as such separated
	//struct Image {
	//	vks::Texture2D texture;
	//	// We also store (and create) a descriptor set that's used to access this texture from the fragment shader
	//	VkDescriptorSet descriptorSet;
	//};

	// A glTF texture stores a reference to the image and a sampler
	// In this sample, we are only interested in the image
	struct Texture {
		int32_t imageIndex;
	};


	~GLTFModel();

	void loadglTFFile(std::string filePath, std::vector<uint32_t>& indices, std::vector<gltf::Vertex>& vertices);

	void loadImages(tinygltf::Model& input);
	void loadTextures(tinygltf::Model& input);
	void loadMaterials(tinygltf::Model& input);
	void loadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, GLTFModel::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer);
	//// Draw a single node including child nodes (if present)
	//void drawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VulkanglTFModel::Node* node);
	//
	//// Draw the glTF scene starting at the top-level-nodes
	//void draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

private:
	/*
		Model data
	*/
	//std::vector<Image> images;
	std::vector<Texture> textures;
	std::vector<Material> materials;
	std::vector<Node*> nodes;
};

}