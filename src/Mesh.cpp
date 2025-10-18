#include <Mesh.hpp>
#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>

Mesh Mesh::generateGrid(float width, float depth, int m, int n)
{
	Mesh mesh;
	mesh.vertices.reserve(m * n * 8);
	mesh.indices.reserve((m - 1) * (n - 1) * 6);


	// Step between vertices
	float dx = width / (m - 1);
	float dz = depth / (n - 1);
	float startX = -width * 0.5f;
	float startZ = -depth * 0.5f;


	// Temp storage positions and uvs to later compute normals
	std::vector<glm::vec3> positions(m * n);
	std::vector<glm::vec2> uvs(m * n);

	for (int i = 0; i < m; ++i) {
		for (int j = 0; j < n; ++j) {
			float x = startX + i * dx;
			float z = startZ + j * dz;

			float scale = 0.5f;
			float amplitude = 3.0f;
			float islandRadius = 0.5f * std::max(width, depth);
			float dist = glm::length(glm::vec2(x, z));
			float falloff = glm::clamp(1.0f - pow(dist / islandRadius, 2.0f), 0.0f, 1.0f);

			// fractal perlin
			float noise = 0.0f;
			float freq = 1.0f;
			float amp = 1.0f;
			for (int o = 0; o < 3; ++o) {
				noise += stb_perlin_noise3(x * scale * freq, 0.0f, z * scale * freq, 0, 0, 0) * amp;
				freq *= 2.0f;
				amp *= 0.5f;
			}

			noise = glm::clamp(noise, -1.0f, 1.0f);
			float y = noise * amplitude * falloff;

			int idx = i * n + j;
			positions[idx] = glm::vec3(x, y, z);
			uvs[idx] = glm::vec2((float)i / (m - 1), (float)j / (n - 1));
		}
	}


	// Compute indices (two triangles per quad)
	for (int i = 0; i < m - 1; ++i) {
		for (int j = 0; j < n - 1; ++j) {
			unsigned int a = i * n + j;
			unsigned int b = (i + 1) * n + j;
			unsigned int c = (i + 1) * n + (j + 1);
			unsigned int d = i * n + (j + 1);
			// triangle 1: a, b, d
			mesh.indices.push_back(a);
			mesh.indices.push_back(b);
			mesh.indices.push_back(d);
			// triangle 2: b, c, d
			mesh.indices.push_back(b);
			mesh.indices.push_back(c);
			mesh.indices.push_back(d);
		}
	}


	// Compute per-vertex normals by averaging face normals
	std::vector<glm::vec3> normals(m * n, glm::vec3(0.0f));
	for (size_t k = 0; k < mesh.indices.size(); k += 3) {
		unsigned int ia = mesh.indices[k + 0];
		unsigned int ib = mesh.indices[k + 1];
		unsigned int ic = mesh.indices[k + 2];
		glm::vec3 A = positions[ia];
		glm::vec3 B = positions[ib];
		glm::vec3 C = positions[ic];
		glm::vec3 faceN = glm::normalize(glm::cross(B - A, C - A));
		normals[ia] += faceN;
		normals[ib] += faceN;
		normals[ic] += faceN;
	}
	for (auto& nrm : normals) nrm = glm::normalize(nrm);


	// Fill interleaved vertex buffer
	for (int i = 0; i < m; ++i) {
		for (int j = 0; j < n; ++j) {
			int idx = i * n + j;
			glm::vec3 p = positions[idx];
			glm::vec3 no = normals[idx];
			glm::vec2 uv = uvs[idx];
			mesh.vertices.push_back(p.x);
			mesh.vertices.push_back(p.y);
			mesh.vertices.push_back(p.z);
			mesh.vertices.push_back(no.x);
			mesh.vertices.push_back(no.y);
			mesh.vertices.push_back(no.z);
			mesh.vertices.push_back(uv.x);
			mesh.vertices.push_back(uv.y);
		}
	}


	mesh.vertexCount = m * n;
	return mesh;
}
