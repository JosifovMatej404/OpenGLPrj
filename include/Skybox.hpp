#pragma once
#include <string>
#include <iostream>
#include <glad/glad.h> // or GLEW

class SkyBox {
    int width, height, channels;
    std::string imagePath;
    unsigned int textureID; // OpenGL texture handle

public:
    
    // Constructor: takes path to the cross image
    SkyBox(const std::string& path);

    unsigned char* extractFace(unsigned char* src, int faceSize, int xOffset, int yOffset, int width, int channels);
    unsigned int loadCubemapFromCross(const std::string& path);

    unsigned int getTextureID() const { return textureID; }
};
