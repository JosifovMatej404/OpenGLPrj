#include <main.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb.h>
#include "Skybox.hpp"


// World dimensions
const float worldWidth = 20000.0f;
const float worldDepth = 20000.0f;

int main() {
    GLFWwindow* window = initGLFW();

    std::string shaderPath = "../res/shaders/";

    std::unordered_map<std::string, std::unique_ptr<Shader>> shaders;
    shaders["terrain"] = std::make_unique<Shader>(shaderPath + "shader.vert", shaderPath + "shader.frag");
    shaders["depth"] = std::make_unique<Shader>(shaderPath + "depth_shader.vert", shaderPath + "depth_shader.frag");
    shaders["sun"] = std::make_unique<Shader>(shaderPath + "sun_shader.vert", shaderPath + "sun_shader.frag");
    shaders["blur"] = std::make_unique<Shader>(shaderPath + "blur.vert", shaderPath + "blur.frag");
    shaders["final"] = std::make_unique<Shader>(shaderPath + "final.vert", shaderPath + "final.frag");
    shaders["brightpass"] = std::make_unique<Shader>(shaderPath + "bright_pass.vert", shaderPath + "bright_pass.frag");
    shaders["skybox"] = std::make_unique<Shader>(shaderPath + "skybox.vert", shaderPath + "skybox.frag");
    shaders["water"] = std::make_unique<Shader>(shaderPath + "water.vert",shaderPath + "water.frag");

    
    Mesh terrain = Mesh::generateGrid(10.0f, 10.0f, 1000, 1000, 10, 0.25f, 0.1f);
    Mesh waterMesh = Mesh::generateWaterPlane(100, 100, worldWidth/10);
    
    glm::vec3 minPos(FLT_MAX, FLT_MAX, FLT_MAX);
    glm::vec3 maxPos(-FLT_MAX, -FLT_MAX, -FLT_MAX);

    // Ensure terrain.vertices.size() is a multiple of 3
    size_t vertexCount = terrain.vertices.size() / 3;

    for (size_t i = 0; i < vertexCount; ++i) {
        float x = terrain.vertices[i * 3 + 0];
        float y = terrain.vertices[i * 3 + 1];
        float z = terrain.vertices[i * 3 + 2];

        if (x < minPos.x) minPos.x = x;
        if (y < minPos.y) minPos.y = y;
        if (z < minPos.z) minPos.z = z;

        if (x > maxPos.x) maxPos.x = x;
        if (y > maxPos.y) maxPos.y = y;
        if (z > maxPos.z) maxPos.z = z;
    }

    glm::vec3 islandCenter = (minPos + maxPos) * 0.5f;

    unsigned int terrainVAO, terrainVBO, terrainEBO;
    unsigned int waterVBO, waterEBO, waterVAO;

    createWaterPlaneVAO(waterMesh.vertices, waterMesh.indices, waterVAO, waterVBO, waterEBO);

    setupMesh(terrain, terrainVAO, terrainVBO, terrainEBO);

    const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
    unsigned int depthMapFBO, depthMap;
    setupShadowMap(depthMapFBO, depthMap, SHADOW_WIDTH, SHADOW_HEIGHT);

    renderLoop(window, shaders, terrain, terrainVAO,
        waterMesh, waterVAO,
        depthMapFBO, depthMap);

    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteBuffers(1, &terrainEBO);

    glDeleteVertexArrays(1, &waterVAO);
    glDeleteBuffers(1, &waterVBO);
    glDeleteBuffers(1, &waterEBO);
    
    glfwTerminate();
    return 0;
}


unsigned int createWaterPlaneVAO(std::vector<float>& outVertices, std::vector<unsigned int>& outIndices,
    unsigned int& VAO, unsigned int& VBO, unsigned int& EBO)
{
    if (outVertices.empty() || outIndices.empty()) {
        std::cerr << "Error: Empty vertex or index buffer!" << std::endl;
        return 0;
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    // Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, outVertices.size() * sizeof(float), outVertices.data(), GL_STATIC_DRAW);

    // Index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, outIndices.size() * sizeof(unsigned int), outIndices.data(), GL_STATIC_DRAW);

    // Stride: 3+3+2+3+3 = 14 floats
    GLsizei stride = 14 * sizeof(float);

    glEnableVertexAttribArray(0); // Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);

    glEnableVertexAttribArray(1); // Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

    glEnableVertexAttribArray(2); // TexCoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    glEnableVertexAttribArray(3); // Tangent
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));

    glEnableVertexAttribArray(4); // Bitangent
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));

    glBindVertexArray(0);

    return VAO;
}


// ------------------- INIT ---------------------
GLFWwindow* initGLFW() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Procedural Terrain with HDR Sun", nullptr, nullptr);
    if (!window) { std::cout << "Failed to create window\n"; glfwTerminate(); exit(-1); }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD\n";
        exit(-1);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    return window;
}

// ------------------- MESH ---------------------
void setupMesh(const Mesh& terrain, unsigned int& VAO, unsigned int& VBO, unsigned int& EBO) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, terrain.vertices.size() * sizeof(float), terrain.vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, terrain.indices.size() * sizeof(unsigned int), terrain.indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

// ------------------- SHADOW MAP ---------------------
void setupShadowMap(unsigned int& depthMapFBO, unsigned int& depthMap, unsigned int width, unsigned int height) {
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ------------------- BLOOM BUFFERS ---------------------
void setupBloomBuffers(BloomBuffers& bloom, unsigned int width, unsigned int height) {
    glGenFramebuffers(1, &bloom.hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, bloom.hdrFBO);

    glGenTextures(2, bloom.colorBuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, bloom.colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, bloom.colorBuffers[i], 0);
    }

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "HDR Framebuffer not complete!\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(2, bloom.pingpongFBO);
    glGenTextures(2, bloom.pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++) {
        glBindFramebuffer(GL_FRAMEBUFFER, bloom.pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, bloom.pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, bloom.pingpongColorbuffers[i], 0);
    }
}

// ------------------- SUN SETUP ---------------------
unsigned int setupSun() {
    unsigned int VAO, VBO, EBO;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    const unsigned int X_SEGMENTS = 32;
    const unsigned int Y_SEGMENTS = 32;

    for (unsigned int y = 0; y <= Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / X_SEGMENTS;
            float ySegment = (float)y / Y_SEGMENTS;
            float xPos = cos(xSegment * 2.0f * M_PI) * sin(ySegment * M_PI);
            float yPos = cos(ySegment * M_PI);
            float zPos = sin(xSegment * 2.0f * M_PI) * sin(ySegment * M_PI);
            vertices.push_back(xPos);
            vertices.push_back(yPos);
            vertices.push_back(zPos);
        }
    }
    for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
        for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            indices.push_back(y * (X_SEGMENTS + 1) + x);
            indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
            indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    return VAO;
}

// ------------------- SKYBOX SETUP ---------------------
unsigned int createSkyboxVAO()
{
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    return skyboxVAO;
}


void renderSkyBox(Shader *shader, unsigned int skyboxVAO, unsigned int cubemapTexture) {
    glDepthFunc(GL_LEQUAL);
    shader->use();

   glm::mat4 view = glm::mat4(glm::mat3(camera.GetViewMatrix()));
   glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom),
       (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

   shader->setMat4("view", view);
   shader->setMat4("projection", projection);

   glBindVertexArray(skyboxVAO);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
   glDrawArrays(GL_TRIANGLES, 0, 36);
   glBindVertexArray(0);
   glDepthFunc(GL_LESS);
}

void renderReflectiveWater(Shader* shader, unsigned int waterVAO, glm::mat4 model, glm::mat4 projection, glm::mat4 view, glm::vec3 pos) {
	shader->use();
	shader->setMat4("projection", projection);
	shader->setMat4("view", view);
	shader->setMat4("model", model);
	shader->setVec3("cameraPos", pos);
	glBindVertexArray(waterVAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}


// ------------------- SUN RENDER ---------------------
void renderSun(Shader* shader, unsigned int sunVAO, glm::vec3 lightDir, glm::mat4 projection, glm::mat4 view) {
    glm::vec3 sunPos = -100.0f * lightDir;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), sunPos);
    model = glm::scale(model, glm::vec3(5.0f));
    shader->use();
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    shader->setVec3("color", glm::vec3(10.0f, 8.0f, 6.0f));
    glBindVertexArray(sunVAO);
    glDrawElements(GL_TRIANGLE_STRIP, 32 * 32 * 6, GL_UNSIGNED_INT, 0);
}
void renderLoop(
    GLFWwindow* window,
    std::unordered_map<std::string, std::unique_ptr<Shader>>& shaders,
    const Mesh& terrain, unsigned int terrainVAO,
    const Mesh& waterMesh, unsigned int waterVAO,
    unsigned int depthMapFBO, unsigned int depthMap)
{
    // ---------------- INITIAL SETUP ----------------
    BloomBuffers bloom;
    setupBloomBuffers(bloom, SCR_WIDTH, SCR_HEIGHT);

    unsigned int sunVAO = setupSun();
    unsigned int skyboxVAO = createSkyboxVAO();

    SkyBox skybox("../res/textures/Daylight Box UV.png");
    unsigned int cubemapTexture = skybox.getTextureID();

    shaders["skybox"]->use();
    shaders["skybox"]->setInt("skybox", 0);

    unsigned int waterNormalMap = loadWaterNormalMap("../res/textures/WaterNormalMap.jpg");

    shaders["water"]->use();
    shaders["water"]->setInt("reflectionTex", 0);
    shaders["water"]->setInt("shadowMap", 1);
    shaders["water"]->setFloat("minHeight", 0.005f);
    shaders["water"]->setFloat("maxHeight", 0.05f);

    // ---------------- REFLECTION FBO ----------------
    unsigned int reflectionFBO, reflectionTex, reflectionRBO;
    glGenFramebuffers(1, &reflectionFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);

    glGenTextures(1, &reflectionTex);
    glBindTexture(GL_TEXTURE_2D, reflectionTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, reflectionTex, 0);

    glGenRenderbuffers(1, &reflectionRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, reflectionRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, reflectionRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    glm::mat4 terrainModel = glm::mat4(1.0f);
    float waterHeight = 0.01f;
    glm::mat4 waterModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, waterHeight, 0.0f));

    // ==================== MAIN LOOP ====================
    while (!glfwWindowShouldClose(window))
    {
        float time = (float)glfwGetTime();
        deltaTime = time - lastFrame;
        lastFrame = time;
        processInput(window);

        // ---------------- LIGHT SETUP ----------------
        glm::vec3 lightDir = glm::normalize(glm::vec3(
            sin(time * 0.1f),
            -1.0f,
            -1.0f
        ));
        glm::vec3 lightPos = -10.0f * lightDir;

        glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, 1.0f, 30.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // ================= SHADOW PASS =================
        glViewport(0, 0, 4096, 4096);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        shaders["depth"]->use();
        shaders["depth"]->setMat4("lightSpaceMatrix", lightSpaceMatrix);

        shaders["depth"]->setMat4("model", terrainModel);
        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, terrain.indices.size(), GL_UNSIGNED_INT, 0);

        shaders["depth"]->setMat4("model", waterModel);
        glBindVertexArray(waterVAO);
        glDrawElements(GL_TRIANGLES, waterMesh.indices.size(), GL_UNSIGNED_INT, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ================= REFLECTION CAMERA =================
        glm::vec3 reflCamPos = camera.Position;
        reflCamPos.y = 2.0f * waterHeight - camera.Position.y;

        float reflPitch = -camera.Pitch;
        float yaw = camera.Yaw;

        glm::vec3 reflFront;
        reflFront.x = cos(glm::radians(yaw)) * cos(glm::radians(reflPitch));
        reflFront.y = sin(glm::radians(reflPitch));
        reflFront.z = sin(glm::radians(yaw)) * cos(glm::radians(reflPitch));
        reflFront = glm::normalize(reflFront);

        glm::mat4 reflView = glm::lookAt(
            reflCamPos,
            reflCamPos + reflFront,
            glm::vec3(0, 1, 0)
        );

        glm::mat4 reflProjection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,
            200.0f
        );

        // ================= REFLECTION PASS =================
        glBindFramebuffer(GL_FRAMEBUFFER, reflectionFBO);
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        glm::mat4 reflectionVP = reflProjection * reflView;
		shaders["water"]->use();
        shaders["water"]->setMat4("reflectionVP", reflectionVP);
        
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        shaders["water"]->setInt("skyCubemap", 3);
        shaders["water"]->setFloat("normalStrength", 0.2f);

        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, waterNormalMap);
        shaders["water"]->setInt("normalMap", 4);
        shaders["water"]->setFloat("normalStrength", 0.1f);
		shaders["water"]->setFloat("time", time);
		shaders["water"]->setVec3("islandPos", glm::vec3(0.0f, 0.0f, 0.0f));
        //CHECK CHATGPT, ADDED WATER NORMAL MAP BUT 

        // Terrain
        shaders["terrain"]->use();
        shaders["terrain"]->setMat4("projection", reflProjection);
        shaders["terrain"]->setMat4("view", reflView);
        shaders["terrain"]->setMat4("model", terrainModel);
        shaders["terrain"]->setVec3("viewPos", reflCamPos);
        shaders["terrain"]->setVec3("lightDir", lightDir);
        shaders["terrain"]->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        shaders["terrain"]->setFloat("clipHeight", waterHeight);
        shaders["terrain"]->setInt("clipAbove", 1);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        shaders["terrain"]->setInt("shadowMap", 1);

        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, terrain.indices.size(), GL_UNSIGNED_INT, 0);

        // Sun (important!)
        renderSun(shaders["sun"].get(), sunVAO, lightDir, reflProjection, reflView);

        glEnable(GL_CULL_FACE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // ================= SCENE PASS =================
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, bloom.hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(
            glm::radians(camera.Zoom),
            (float)SCR_WIDTH / (float)SCR_HEIGHT,
            0.1f,
            200.0f
        );

        renderSkyBox(shaders["skybox"].get(), skyboxVAO, cubemapTexture);

        shaders["terrain"]->use();
        shaders["terrain"]->setMat4("projection", projection);
        shaders["terrain"]->setMat4("view", view);
        shaders["terrain"]->setMat4("model", terrainModel);
        shaders["terrain"]->setVec3("viewPos", camera.Position);
        shaders["terrain"]->setVec3("lightDir", lightDir);
        shaders["terrain"]->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        shaders["terrain"]->setFloat("clipHeight", waterHeight);
        shaders["terrain"]->setInt("clipAbove", -1); // disable clipping

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        shaders["terrain"]->setInt("shadowMap", 1);

        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, terrain.indices.size(), GL_UNSIGNED_INT, 0);

        // ================= WATER PASS =================
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);

        shaders["water"]->use();
        shaders["water"]->setMat4("projection", projection);
        shaders["water"]->setMat4("view", view);
        shaders["water"]->setMat4("model", waterModel);
        shaders["water"]->setVec3("viewPos", camera.Position);
        shaders["water"]->setVec3("lightDir", lightDir);
        shaders["water"]->setFloat("time", time);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, reflectionTex);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);

        glBindVertexArray(waterVAO);
        glDrawElements(GL_TRIANGLES, waterMesh.indices.size(), GL_UNSIGNED_INT, 0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);

        // ================= SUN =================
        renderSun(shaders["sun"].get(), sunVAO, lightDir, projection, view);

        // ================= BLOOM =================
        glBindFramebuffer(GL_FRAMEBUFFER, bloom.pingpongFBO[0]);
        shaders["brightpass"]->use();
        shaders["brightpass"]->setFloat("threshold", 0.6f);
        glBindTexture(GL_TEXTURE_2D, bloom.colorBuffers[0]);
        renderQuad();

        bool horizontal = true, first = true;
        shaders["blur"]->use();
        for (int i = 0; i < 5; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, bloom.pingpongFBO[horizontal]);
            shaders["blur"]->setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D,
                first ? bloom.pingpongColorbuffers[0]
                : bloom.pingpongColorbuffers[!horizontal]);
            renderQuad();
            horizontal = !horizontal;
            if (first) first = false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shaders["final"]->use();
        shaders["final"]->setBool("bloom", true);
        shaders["final"]->setFloat("exposure", 1.3f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bloom.colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, bloom.pingpongColorbuffers[!horizontal]);

        renderQuad();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}



void renderQuad() {
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texCoords
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        };

        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

unsigned int loadWaterNormalMap(const char* path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data)
    {
        GLenum format = (nrChannels == 3) ? GL_RGB : GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cerr << "Failed to load normal map: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}


void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
void mouse_callback(GLFWwindow* window, double xposd, double yposd) {
    float xpos = static_cast<float>(xposd);
    float ypos = static_cast<float>(yposd);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    camera.ProcessMouseMovement(xoffset, yoffset);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}
