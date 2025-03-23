#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;

#define   WIDTH   1400
#define   HEIGHT   1000 

const int MAX_FRAMES_IN_FLIGHT = 2;
const int PARTICLE_NUM = 2000;

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


struct Particle {
	vec2 position;
	vec2 velocity;
	vec4 color;
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