#ifndef mMesh
#define mMesh
#pragma once

#include <vector>
#include <glm/glm.hpp>

class Mesh {
public:
	std::vector<float> vertices; // interleaved: pos(x,y,z), normal(x,y,z), uv(u,v)
	std::vector<unsigned int> indices;
	int vertexCount;

	static Mesh generateGrid(float width, float depth, int m, int n);
};

#endif