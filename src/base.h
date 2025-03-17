#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace base {
struct Vertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec3 color;

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && normal == other.normal;
	}
};

struct UniformBufferObject
{
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
};

}

namespace std {
	template<> struct hash<base::Vertex> {
		size_t operator()(base::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec3>()(vertex.normal) << 1);
		}
	};
}