#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>

#include <cstdio>
#include <memory>

#include <Engine/Headers/shaderHelper.h>
#include <stb_image.h>
#include <unordered_map>

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <vec3.hpp>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include <Engine/Headers/camera.h>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

void processInput(GLFWwindow* window);
void LoadTexture(int& width, int& height, int& nrChannels, unsigned int& texture, const char* file, bool hasAlpha);
void initializeImgui(GLFWwindow* window);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

GLFWwindow* initializeWindow();

Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    std::string diffuseMap;
    std::string diffuseTexture;
    unsigned int textureID; // Texture ID
};

struct Face {
    glm::vec3 vertices[3];
    glm::vec2 uvs[3];
    glm::vec3 normals[3];
};

struct ModelPart {
    unsigned int vertexCount;
    std::string materialName;
    std::vector<Face> faces;
};

std::unordered_map<std::string, Material> loadMTL(const std::string& mtlPath) {
    std::unordered_map<std::string, Material> materials;
    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open MTL file: " << mtlPath << std::endl;
        return materials;
    }

    Material currentMaterial;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;

        unsigned int textureID = 0;

        if (type == "newmtl") {
            if (currentMaterial.diffuseMap != "") {
                materials[currentMaterial.diffuseMap] = currentMaterial;
            }
            currentMaterial = Material();
            iss >> currentMaterial.diffuseMap;
        }
        else if (type == "Ka") {
            iss >> currentMaterial.ambient.r >> currentMaterial.ambient.g >> currentMaterial.ambient.b;
        }
        else if (type == "Kd") {
            iss >> currentMaterial.diffuse.r >> currentMaterial.diffuse.g >> currentMaterial.diffuse.b;
        }
        else if (type == "Ks") {
            iss >> currentMaterial.specular.r >> currentMaterial.specular.g >> currentMaterial.specular.b;
        }
        else if (type == "Ns") {
            iss >> currentMaterial.shininess;
        }
        else if (type == "map_Kd") {
            iss >> currentMaterial.diffuseTexture;
            int width, height, nrChannels;
            std::string img = "../Engine/Source/Engine/Images/" + currentMaterial.diffuseTexture;
            textureID++;
            LoadTexture(width, height, nrChannels, textureID, img.c_str(), false);
            currentMaterial.textureID = textureID;
        }
    }

    if (currentMaterial.diffuseMap != "") {
        materials[currentMaterial.diffuseMap] = currentMaterial;
    }

    return materials;
}

bool loadOBJ(
    const std::string& path,
    std::vector<ModelPart>& out_model_parts
) {
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices, temp_normals;
    std::vector<glm::vec2> temp_uvs;
    std::string currentMaterialName;
    ModelPart* currentModelPart = nullptr;

    FILE* file = nullptr;
    errno_t err = fopen_s(&file, path.c_str(), "r");
    std::unique_ptr<FILE, decltype(&fclose)> file_ptr(file, &fclose);
    if (err != 0 || !file_ptr) {
        std::cerr << "The file '" << path << "' could not be opened\n";
        return false;
    }

    char line[128];

    while (fgets(line, sizeof(line), file_ptr.get())) {
        if (strncmp(line, "v ", 2) == 0) {
            glm::vec3 vertex;
            if (sscanf_s(line + 2, "%f %f %f", &vertex.x, &vertex.y, &vertex.z) == 3) {
                temp_vertices.push_back(vertex);
            }
        }
        else if (strncmp(line, "vt ", 3) == 0) {
            glm::vec2 uv;
            if (sscanf_s(line + 3, "%f %f", &uv.x, &uv.y) == 2) {
                // Invert V coordinate to conform with OpenGL standards
                uv.y = 1.0f - uv.y;
                temp_uvs.push_back(uv);
            }
        }
        else if (strncmp(line, "vn ", 3) == 0) {
            glm::vec3 normal;
            if (sscanf_s(line + 3, "%f %f %f", &normal.x, &normal.y, &normal.z) == 3) {
                temp_normals.push_back(normal);
            }
        }
        else if (strncmp(line, "f ", 2) == 0) {
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            int matches = sscanf_s(line + 2, "%d/%d/%d %d/%d/%d %d/%d/%d",
                &vertexIndex[0], &uvIndex[0], &normalIndex[0],
                &vertexIndex[1], &uvIndex[1], &normalIndex[1],
                &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
            if (matches != 9) {
                std::cerr << "File can't be read by our simple parser. Try exporting with other options.\n";
                return false;
            }
            if (currentModelPart == nullptr) {
                out_model_parts.emplace_back();
                currentModelPart = &out_model_parts.back();
                currentModelPart->materialName = currentMaterialName;
            }
            Face face;
            for (int i = 0; i < 3; ++i) {
                face.vertices[i] = temp_vertices[vertexIndex[i] - 1];
                face.uvs[i] = temp_uvs[uvIndex[i] - 1];
                face.normals[i] = temp_normals[normalIndex[i] - 1];
            }
            currentModelPart->faces.push_back(face);
            currentModelPart->vertexCount += 3;
        }
        else if (strncmp(line, "usemtl ", 7) == 0) {
            currentMaterialName = line + 7;
            // Remove trailing newline if present
            currentMaterialName.erase(std::remove(currentMaterialName.begin(), currentMaterialName.end(), '\n'), currentMaterialName.end());

            // Create new ModelPart for the new material
            out_model_parts.emplace_back();
            currentModelPart = &out_model_parts.back();
            currentModelPart->materialName = currentMaterialName;
        }
    }

    return true;
}

void setupModelPartBuffers(const ModelPart& part, GLuint& VAO, GLuint& VBO) {
    // Generate buffers
    glGenBuffers(1, &VBO);
    glGenVertexArrays(1, &VAO);

    glBindVertexArray(VAO);

    // Prepare data for VBO
    std::vector<float> vertexData;

    for (const auto& face : part.faces) {
        for (int i = 0; i < 3; ++i) {
            const glm::vec3& vertex = face.vertices[i];
            const glm::vec2& uv = face.uvs[i];
            const glm::vec3& normal = face.normals[i];

            vertexData.insert(vertexData.end(), { vertex.x, vertex.y, vertex.z });
            vertexData.insert(vertexData.end(), { uv.x, uv.y });
            vertexData.insert(vertexData.end(), { normal.x, normal.y, normal.z });
        }
    }

    // Upload vertex data to VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    // Set up vertex attribute pointers
    // Assuming layout is as follows: 
    // position (3 floats), uv (2 floats), normal (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); // Position
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); // UV
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float))); // Normal
    glEnableVertexAttribArray(2);

    // Unbind VAO
    glBindVertexArray(0);
}

void renderModelPart(const ModelPart& part, const std::unordered_map<std::string, Material>& materials, GLuint VAO, Shader& shader) {
    glBindVertexArray(VAO);

    auto it = materials.find(part.materialName);
    if (it != materials.end()) {
        const Material& material = it->second;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.textureID);
        shader.setInt("texture1", 0);
    }

    glDrawArrays(GL_TRIANGLES, 0, part.vertexCount);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}

int main()
{
    GLFWwindow* window = initializeWindow();
    if (window == nullptr) return -1;
    glEnable(GL_DEPTH_TEST);
    initializeImgui(window);


    std::unordered_map<std::string, Material> materials = loadMTL("../Engine/Source/Engine/Models/test.mtl");
    std::vector<Face> faces;
    std::vector<ModelPart> modelParts;

    if (!loadOBJ("../Engine/Source/Engine/Models/test.obj", modelParts)) {
        std::cerr << "Failed to load the object" << std::endl;
        return -1;
    }

    std::vector<GLuint> VAOS;
    std::vector<GLuint> VBOS;

    for (const auto& part : modelParts) {
		GLuint VAO, VBO;
		setupModelPartBuffers(part, VAO, VBO);
		VAOS.push_back(VAO);
		VBOS.push_back(VBO);
	}

    Shader shader("../Engine/Source/Engine/modelShader.vs", "../Engine/Source/Engine/modelShader.fs");

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::ShowDemoWindow(); // Show demo window! :)

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(50.0f) * (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", model);

        int index = 0;
        for (const auto& part : modelParts) {
            renderModelPart(part, materials, VAOS[index], shader);
            index++;
        }

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // delete all vaos and vbos
    for (auto vao : VAOS) {
		glDeleteVertexArrays(1, &vao);
	}

    for (auto vbo : VBOS) {
        glDeleteBuffers(1, &vbo);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}

void LoadTexture(int& width, int& height, int& nrChannels, unsigned int& texture, const char* file, bool hasAlpha)
{
    unsigned char* data = stbi_load(file, &width, &height, &nrChannels, 0);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    if (data)
    {
        if (hasAlpha)
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }

    stbi_image_free(data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void initializeImgui(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

GLFWwindow* initializeWindow() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return nullptr;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Marie Gyro Engine", NULL, NULL);
    if (window == NULL)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return nullptr;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return nullptr;
    }

    return window;
}