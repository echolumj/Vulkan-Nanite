#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace obj {
	struct  Vertex
	{
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec3 color;

		bool operator==(const Vertex& other) const {
			return pos == other.pos && color == other.color && normal == other.normal;
		}
	};

	class Object
	{
	public:
		Object(std::string name);
		~Object();

		std::vector<Vertex> getVertices(void) { return vertices; }
		std::vector<uint32_t> getIndices(void) { return indices; }

	private:
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

	};
}

namespace std {
	template<> struct hash<obj::Vertex> {
		size_t operator()(obj::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
				(hash<glm::vec3>()(vertex.normal) << 1);
		}
	};
}