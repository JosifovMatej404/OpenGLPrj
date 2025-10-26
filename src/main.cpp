#include <main.hpp>
#include <stb.h>

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

    Mesh terrain = Mesh::generateGrid(10.0f, 10.0f, 1000, 1000);
    unsigned int VAO, VBO, EBO;
    setupMesh(terrain, VAO, VBO, EBO);

    const unsigned int SHADOW_WIDTH = 4096, SHADOW_HEIGHT = 4096;
    unsigned int depthMapFBO, depthMap;
    setupShadowMap(depthMapFBO, depthMap, SHADOW_WIDTH, SHADOW_HEIGHT);

    renderLoop(window, shaders, terrain, VAO, depthMapFBO, depthMap);

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glfwTerminate();
    return 0;
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

// ------------------- MAIN RENDER LOOP ---------------------
void renderLoop(GLFWwindow* window,
    std::unordered_map<std::string, std::unique_ptr<Shader>>& shaders,
    const Mesh& terrain, unsigned int VAO,
    unsigned int depthMapFBO, unsigned int depthMap)
{
    BloomBuffers bloom;
    setupBloomBuffers(bloom, SCR_WIDTH, SCR_HEIGHT);
    unsigned int sunVAO = setupSun();

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window);

        glm::vec3 lightDir = glm::normalize(glm::vec3(glm::sin(glfwGetTime() * 0.1f), -1.0f, -1.0f));
        glm::vec3 lightPos = -10.0f * lightDir;

        glm::mat4 lightProjection = glm::ortho(-15.0f, 15.0f, -15.0f, 15.0f, 1.0f, 15.0f);
        glm::mat4 lightView = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        // --- Shadow pass ---
        shaders["depth"]->use();
        shaders["depth"]->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        glViewport(0, 0, 4096, 4096);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glm::mat4 model(1.0f);
        shaders["depth"]->setMat4("model", model);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, terrain.indices.size(), GL_UNSIGNED_INT, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- HDR scene render ---
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, bloom.hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        shaders["terrain"]->use();
        shaders["terrain"]->setMat4("projection", projection);
        shaders["terrain"]->setMat4("view", view);
        shaders["terrain"]->setMat4("model", model);
        shaders["terrain"]->setVec3("viewPos", camera.Position);
        shaders["terrain"]->setVec3("lightDir", lightDir);
        shaders["terrain"]->setMat4("lightSpaceMatrix", lightSpaceMatrix);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        shaders["terrain"]->setInt("shadowMap", 1);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, terrain.indices.size(), GL_UNSIGNED_INT, 0);

        renderSun(shaders["sun"].get(), sunVAO, lightDir, projection, view);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- Bright-pass extraction ---
        glBindFramebuffer(GL_FRAMEBUFFER, bloom.pingpongFBO[0]);
        glClear(GL_COLOR_BUFFER_BIT);
        shaders["brightpass"]->use();
        shaders["brightpass"]->setInt("scene", 0);
        shaders["brightpass"]->setFloat("threshold", 0.9f);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bloom.colorBuffers[0]);
        renderQuad();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- Gaussian blur ---
        bool horizontal = true, first_iteration = true;
        shaders["blur"]->use();
        for (unsigned int i = 0; i < 5; i++) {
            glBindFramebuffer(GL_FRAMEBUFFER, bloom.pingpongFBO[horizontal]);
            shaders["blur"]->setInt("horizontal", horizontal);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? bloom.pingpongColorbuffers[0]
                : bloom.pingpongColorbuffers[!horizontal]);
            horizontal = !horizontal;
            renderQuad();
            if (first_iteration) first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // --- Final combine and tone map ---
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shaders["final"]->use();
        shaders["final"]->setInt("scene", 0);
        shaders["final"]->setInt("bloomBlur", 1);
        shaders["final"]->setBool("bloom", true);
        shaders["final"]->setFloat("exposure", 1);
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
