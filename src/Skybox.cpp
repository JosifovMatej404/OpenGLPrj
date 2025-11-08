#include "Skybox.hpp"
#include <stb_image.h>
#include <vector>

SkyBox::SkyBox(const std::string& path)
    : width(0), height(0), channels(0), textureID(0), imagePath(path)
{
    textureID = loadCubemapFromCross(path);
}

unsigned int SkyBox::loadCubemapFromCross(const std::string& path)
{
    int w, h, c;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 0);
    if (!data) {
        std::cout << "Failed to load skybox image: " << path << "\n";
        return 0;
    }

    int faceSize = h / 3; // 3 rows in your cross
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    // Map your layout: row 0 = top, row1 = middle, row2 = bottom
    // columns 0..3
    std::vector<std::pair<int, int>> offsets = {
        {2, 1}, // POSITIVE_X
        {0, 1}, // NEGATIVE_X
        {1, 0}, // POSITIVE_Y (top)
        {1, 2}, // NEGATIVE_Y (bottom)
        {1, 1}, // POSITIVE_Z (front)
        {3, 1}  // NEGATIVE_Z (back)
    };

    for (unsigned int i = 0; i < 6; ++i)
    {
        int col = offsets[i].first;
        int row = offsets[i].second;

        unsigned char* faceData = extractFace(data, faceSize, col * faceSize, row * faceSize, w, c);
        if (!faceData) {
            std::cout << "Failed to extract face " << i << "\n";
            continue;
        }

        GLenum format = (c == 3) ? GL_RGB : GL_RGBA;

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format,
            faceSize, faceSize, 0, format, GL_UNSIGNED_BYTE, faceData);
        delete[] faceData;
    }

    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned char* SkyBox::extractFace(unsigned char* src, int faceSize, int xOffset, int yOffset, int imgWidth, int channels)
{
    unsigned char* face = new unsigned char[faceSize * faceSize * channels];
    for (int y = 0; y < faceSize; ++y) {
        for (int x = 0; x < faceSize; ++x) {
            for (int c = 0; c < channels; ++c) {
                int srcIndex = ((y + yOffset) * imgWidth + (x + xOffset)) * channels + c;
                int dstIndex = (y * faceSize + x) * channels + c;
                face[dstIndex] = src[srcIndex];
            }
        }
    }
    return face;
}