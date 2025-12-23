#include <Mesh.hpp>
#define STB_PERLIN_IMPLEMENTATION
#include <stb_perlin.h>
#include <random>

static const float SEA_LEVEL = 0.0f;
static const float SHORE_WIDTH = 0.0f;

Mesh Mesh::generateGrid(float width, float depth, int m, int n,
    int erosionIterations = 20, float hydraulicFactor = 0.5f,
    float talusAngle = 0.2f)
{
    Mesh mesh;
    mesh.vertices.reserve(m * n * 8);
    mesh.indices.reserve((m - 1) * (n - 1) * 6);

    float dx = width / (m - 1);
    float dz = depth / (n - 1);
    float startX = -width * 0.5f;
    float startZ = -depth * 0.5f;

    std::vector<glm::vec3> positions(m * n);
    std::vector<glm::vec2> uvs(m * n);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1000.0f);
    float offsetX = dis(gen);
    float offsetZ = dis(gen);

    float islandRadius = 0.4f * std::max(width, depth);

    // --- Generate initial heightmap ---
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            float x = startX + i * dx;
            float z = startZ + j * dz;

            float scale = 0.5f;
            float amplitude = 3.0f;
            float dist = glm::length(glm::vec2(x, z));
            float falloff = glm::clamp(1.0f - (dist / islandRadius) * (dist / islandRadius), 0.0f, 1.0f);

            float noise = 0.0f;
            float freq = 1.0f;
            float amp = 1.0f;
            for (int o = 0; o < 3; ++o) {
                noise += stb_perlin_noise3(
                    (x + offsetX) * scale * freq,
                    0.0f,
                    (z + offsetZ) * scale * freq,
                    0, 0, 0
                ) * amp;
                freq *= 2.0f;
                amp *= 0.5f;
            }
            noise = glm::clamp(noise, -1.0f, 1.0f);
            float rawHeight = noise * amplitude * falloff;

            float t = glm::smoothstep(
                SEA_LEVEL - SHORE_WIDTH,
                SEA_LEVEL + SHORE_WIDTH,
                rawHeight
            );

            float y = glm::mix(SEA_LEVEL, rawHeight, t);

            int idx = i * n + j;
            positions[idx] = glm::vec3(x, y, z);
            uvs[idx] = glm::vec2((float)i / (m - 1), (float)j / (n - 1));
        }
    }

    std::vector<float> newHeights(m * n);

    for (int iter = 0; iter < erosionIterations; ++iter) {
        // Copy current heights
        for (int k = 0; k < m * n; ++k)
            newHeights[k] = positions[k].y;

        for (int i = 1; i < m - 1; ++i) {
            for (int j = 1; j < n - 1; ++j) {
                int idx = i * n + j;
                float centerY = positions[idx].y;

                float totalDelta = 0.0f;
                std::vector<float> deltas(8, 0.0f);

                // Check 8 neighbors
                int nIdx = 0;
                for (int ni = -1; ni <= 1; ++ni) {
                    for (int nj = -1; nj <= 1; ++nj) {
                        if (ni == 0 && nj == 0) continue;
                        int neighborIdx = (i + ni) * n + (j + nj);
                        float neighborY = positions[neighborIdx].y;
                        float delta = centerY - neighborY;
                        if (delta > 0.01f) {
                            delta *= hydraulicFactor * 0.5f;
                            delta = glm::min(delta, 0.05f); // max move
                            deltas[nIdx++] = delta;
                            totalDelta += delta;
                        }
                        else {
                            deltas[nIdx++] = 0.0f;
                        }
                    }
                }

                // Subtract total from center
                newHeights[idx] -= totalDelta;

                // Distribute to neighbors proportionally
                nIdx = 0;
                for (int ni = -1; ni <= 1; ++ni) {
                    for (int nj = -1; nj <= 1; ++nj) {
                        if (ni == 0 && nj == 0) continue;
                        int neighborIdx = (i + ni) * n + (j + nj);
                        if (totalDelta > 0.0f)
                            newHeights[neighborIdx] += deltas[nIdx++] * (deltas[nIdx - 1] / totalDelta);
                    }
                }
            }
        }

        // Copy new heights back
        for (int k = 0; k < m * n; ++k)
            positions[k].y = newHeights[k];
    }

    // --- Thermal erosion (slope-based smoothing) ---
    for (int iter = 0; iter < 3; ++iter) {
        std::vector<glm::vec3> newPositions = positions;
        for (int i = 1; i < m - 1; ++i) {
            for (int j = 1; j < n - 1; ++j) {
                int idx = i * n + j;
                float centerY = positions[idx].y;

                for (int ni = -1; ni <= 1; ++ni) {
                    for (int nj = -1; nj <= 1; ++nj) {
                        if (ni == 0 && nj == 0) continue;
                        int nIdx = (i + ni) * n + (j + nj);
                        float neighborY = positions[nIdx].y;
                        float slope = centerY - neighborY;

                        if (slope > talusAngle) {
                            float move = (slope - talusAngle) * 0.5f;
                            newPositions[idx].y -= move;
                            newPositions[nIdx].y += move;
                        }
                    }
                }
            }
        }
        positions = newPositions;
    }

    // --- Generate indices and normals (same as before) ---
    for (int i = 0; i < m - 1; ++i) {
        for (int j = 0; j < n - 1; ++j) {
            unsigned int a = i * n + j;
            unsigned int b = (i + 1) * n + j;
            unsigned int c = (i + 1) * n + (j + 1);
            unsigned int d = i * n + (j + 1);

            auto inside = [&](unsigned int idx) {
                glm::vec2 posXZ(positions[idx].x, positions[idx].z);
                return glm::length(posXZ) <= islandRadius;
                };

            if (inside(a) || inside(b) || inside(c)) {
                mesh.indices.push_back(a);
                mesh.indices.push_back(d);
                mesh.indices.push_back(b);
            }

            if (inside(b) || inside(c) || inside(d)) {
                mesh.indices.push_back(d);
                mesh.indices.push_back(c);
                mesh.indices.push_back(b);
            }
        }
    }

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

    for (size_t i = 0; i < normals.size(); ++i)
        normals[i] = glm::normalize(normals[i]);

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


Mesh Mesh::generateWaterPlane(float width, float depth, unsigned int divisions = 1)
{
    Mesh mesh;
    mesh.vertices.clear();
    mesh.indices.clear();

    const float WATER_OFFSET = 0.02f;
    float y = SEA_LEVEL - WATER_OFFSET;

    float halfW = width * 0.5f;
    float halfD = depth * 0.5f;

    float dx = width / divisions;
    float dz = depth / divisions;

    glm::vec3 upNormal(0.0f, 1.0f, 0.0f);
    glm::vec3 tangent(1.0f, 0.0f, 0.0f);
    glm::vec3 bitangent(0.0f, 0.0f, 1.0f);

    // Create vertices
    for (unsigned int z = 0; z <= divisions; ++z)
    {
        for (unsigned int x = 0; x <= divisions; ++x)
        {
            glm::vec3 pos(-halfW + x * dx, y, -halfD + z * dz);
            glm::vec2 uv((float)x / divisions, (float)z / divisions);

            // Push vertex attributes: pos, normal, uv, tangent, bitangent
            mesh.vertices.push_back(pos.x);
            mesh.vertices.push_back(pos.y);
            mesh.vertices.push_back(pos.z);

            mesh.vertices.push_back(upNormal.x);
            mesh.vertices.push_back(upNormal.y);
            mesh.vertices.push_back(upNormal.z);

            mesh.vertices.push_back(uv.x);
            mesh.vertices.push_back(uv.y);

            mesh.vertices.push_back(tangent.x);
            mesh.vertices.push_back(tangent.y);
            mesh.vertices.push_back(tangent.z);

            mesh.vertices.push_back(bitangent.x);
            mesh.vertices.push_back(bitangent.y);
            mesh.vertices.push_back(bitangent.z);
        }
    }

    // Create indices
    for (unsigned int z = 0; z < divisions; ++z)
    {
        for (unsigned int x = 0; x < divisions; ++x)
        {
            unsigned int topLeft = z * (divisions + 1) + x;
            unsigned int bottomLeft = (z + 1) * (divisions + 1) + x;
            unsigned int topRight = topLeft + 1;
            unsigned int bottomRight = bottomLeft + 1;

            mesh.indices.push_back(topLeft);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(topRight);

            mesh.indices.push_back(topRight);
            mesh.indices.push_back(bottomLeft);
            mesh.indices.push_back(bottomRight);
        }
    }

    mesh.vertexCount = (divisions + 1) * (divisions + 1);

    return mesh;
}

