#ifndef mMain
#define mMain
#pragma once


#include <OpenGLPrj.hpp>
#include <GLFW/glfw3.h>
#include <Camera.hpp>
#include <Shader.hpp>
#include <Mesh.hpp>
#include <stb_perlin.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

Camera camera(glm::vec3(0.0f, 5.0f, 10.0f));

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

unsigned int quadVAO = 0;
unsigned int quadVBO;

std::unordered_map<std::string, Shader> shaders;

struct BloomBuffers {
    unsigned int hdrFBO;
    unsigned int colorBuffers[2];
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
};

// Function declarations
// Main
int main();

// GLFW / OpenGL init
GLFWwindow* initGLFW();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Mesh & Shadow setup
void setupMesh(const Mesh& terrain, unsigned int& VAO, unsigned int& VBO, unsigned int& EBO);
void setupShadowMap(unsigned int& depthMapFBO, unsigned int& depthMap, const unsigned int SHADOW_WIDTH, const unsigned int SHADOW_HEIGHT);
unsigned int setupSun();

// Render helpers
void setupBloomBuffers(BloomBuffers& bloom, unsigned int width, unsigned int height);
void renderSun(Shader* sunShader, unsigned int sunVAO, glm::vec3 lightDir, glm::mat4 projection, glm::mat4 view);

// Render loop
void renderLoop(GLFWwindow* window, std::unordered_map<std::string, std::unique_ptr<Shader>>& shaders,
    const Mesh& terrain, unsigned int VAO, unsigned int depthMapFBO, unsigned int depthMap);
void renderQuad();

// Input
void processInput(GLFWwindow* window);
#endif