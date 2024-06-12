#include "Engine/Headers/Engine.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>

#include <filesystem>

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
    std::string diffuseMap; // Filename of the diffuse texture map
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
    }

    if (currentMaterial.diffuseMap != "") {
        materials[currentMaterial.diffuseMap] = currentMaterial;
    }

    return materials;
}

struct Face {
    unsigned int vertexIndex[3];
    unsigned int uvIndex[3];
    unsigned int normalIndex[3];
    std::string materialName;
};

bool loadOBJ(
    const std::string& path,
    std::vector<glm::vec3>& out_vertices,
    std::vector<glm::vec2>& out_uvs,
    std::vector<glm::vec3>& out_normals,
    std::vector<Face>& out_faces
) {
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices, temp_normals;
    std::vector<glm::vec2> temp_uvs;
    std::string currentMaterialName;

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
            vertexIndices.push_back(vertexIndex[0]);
            vertexIndices.push_back(vertexIndex[1]);
            vertexIndices.push_back(vertexIndex[2]);
            uvIndices.push_back(uvIndex[0]);
            uvIndices.push_back(uvIndex[1]);
            uvIndices.push_back(uvIndex[2]);
            normalIndices.push_back(normalIndex[0]);
            normalIndices.push_back(normalIndex[1]);
            normalIndices.push_back(normalIndex[2]);

            // Store material name for each face
            out_faces.emplace_back();
            out_faces.back().materialName = currentMaterialName;
            for (int i = 0; i < 3; ++i) {
                out_faces.back().vertexIndex[i] = vertexIndex[i];
                out_faces.back().uvIndex[i] = uvIndex[i];
                out_faces.back().normalIndex[i] = normalIndex[i];
            }
        }
        else if (strncmp(line, "usemtl ", 7) == 0) {
            currentMaterialName = line + 7;
            // Remove trailing newline if present
            currentMaterialName.erase(std::remove(currentMaterialName.begin(), currentMaterialName.end(), '\n'), currentMaterialName.end());
        }
    }

    // Populate out_vertices, out_uvs, out_normals using indices
    for (size_t i = 0; i < vertexIndices.size(); ++i) {
        unsigned int vertexIndex = vertexIndices[i];
        out_vertices.push_back(temp_vertices[vertexIndex - 1]);

        if (!uvIndices.empty()) {
            unsigned int uvIndex = uvIndices[i];
            out_uvs.push_back(temp_uvs[uvIndex - 1]);
        }

        if (!normalIndices.empty()) {
            unsigned int normalIndex = normalIndices[i];
            out_normals.push_back(temp_normals[normalIndex - 1]);
        }
    }

    return true;
}

int main()
{
    GLFWwindow* window = initializeWindow();
    if (window == nullptr) return -1;
    glEnable(GL_DEPTH_TEST);
    initializeImgui(window);

    /*

    // texture loading

    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned int texture, texture2;
    LoadTexture(width, height, nrChannels, texture, "Images/wall.jpg", false);
    LoadTexture(width, height, nrChannels, texture2, "Images/awesomeface.png", true);

    Shader RectShader("shader.vs", "shader.fs");

    float vertices[] = {
         // positions         // colors          // texture coords
        -0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, // Bottom-left
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, // Bottom-right
         0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Top-right
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, // Top-left

        -0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f, // Bottom-left
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f,  1.0f, 0.0f, // Bottom-right
         0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f, // Top-right
        -0.5f,  0.5f, -0.5f,  0.0f, 0.0f, 1.0f,  0.0f, 1.0f, // Top-left
    };

    unsigned int indices[] = {
        // Front face
        0, 1, 2,
        2, 3, 0,
        // Right face
        1, 5, 6,
        6, 2, 1,
        // Back face
        7, 6, 5,
        5, 4, 7,
        // Left face
        4, 0, 3,
        3, 7, 4,
        // Bottom face
        4, 5, 1,
        1, 0, 4,
        // Top face
        3, 2, 6,
        6, 7, 3
    };

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // verts
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // colour
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // texture
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    //clear
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    RectShader.use();

    RectShader.setInt("texture", 0);
    RectShader.setInt("texture2", 1);

    */

    // load Materials
    std::filesystem::path currentDir = std::filesystem::current_path();
    std::cout << "Current directory is: " << currentDir << std::endl;
    std::unordered_map<std::string, Material> materials = loadMTL("../Engine/Source/Engine/Models/test.mtl");

    // load model
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;
    std::vector<Face> faces;

    if (!loadOBJ("../Engine/Source/Engine/Models/test.obj", vertices, uvs, normals, faces)) {
        std::cerr << "Failed to load the object" << std::endl;
        return -1;
    }


    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

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

        for (const auto& face : faces) {
            // Retrieve the material associated with this face
            auto materialIt = materials.find(face.materialName);
            if (materialIt != materials.end()) {
                const Material& material = materialIt->second;

                // Bind material properties to shader uniforms
                shader.setVec3("material.ambient", material.ambient);
                shader.setVec3("material.diffuse", material.diffuse);
                shader.setVec3("material.specular", material.specular);
                shader.setFloat("material.shininess", material.shininess);
            }
        }

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        glBindVertexArray(0);






        //glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        //RectShader.setMat4("projection", projection);
        //glm::mat4 view = camera.GetViewMatrix();
        //RectShader.setMat4("view", view);


        //glm::mat4 modelMatrix = glm::mat4(1.0f);
        //modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        //modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * glm::radians(50.0f), glm::vec3(0.5f, 1.0f, 0.0f));
        //RectShader.setMat4("model", modelMatrix);

        //draw funny rect
        //glActiveTexture(GL_TEXTURE0);
        //glBindTexture(GL_TEXTURE_2D, texture);
        //glActiveTexture(GL_TEXTURE1);
        //glBindTexture(GL_TEXTURE_2D, texture2);

        //RectShader.use();

       // glBindVertexArray(VAO);
       // glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);

    //glDeleteBuffers(1, &EBO);

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
