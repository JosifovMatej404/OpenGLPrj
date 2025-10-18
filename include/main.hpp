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


const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 800;

Camera camera(glm::vec3(0.0f, 5.0f, 10.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Function declarations
GLFWwindow* initGLFW();
void setupMesh(const Mesh& terrain, unsigned int& VAO, unsigned int& VBO, unsigned int& EBO);
void setupShadowMap(unsigned int& depthMapFBO, unsigned int& depthMap, const unsigned int SHADOW_WIDTH, const unsigned int SHADOW_HEIGHT);
void renderLoop(GLFWwindow* window, Shader& shader, Shader& depthShader, const Mesh& terrain, unsigned int VAO, unsigned int depthMapFBO, unsigned int depthMap);

void processInput(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xposd, double yposd);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

#endif