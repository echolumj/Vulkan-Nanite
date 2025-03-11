#pragma once
#include <vector>
#include <string>

#include "base.h"

namespace obj {

	class Object
	{
	public:
		Object(std::string name);
		~Object();

		std::vector<base::Vertex> getVertices(void) { return vertices; }
		std::vector<uint32_t> getIndices(void) { return indices; }

	private:
		std::vector<base::Vertex> vertices;
		std::vector<uint32_t> indices;

	};
}
