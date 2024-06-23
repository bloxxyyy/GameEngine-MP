#include "objLoader.h"

#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <stb_image.h>
#include <glad/glad.h>
#include <shaderHelper.h>
#include <ext/matrix_clip_space.hpp>
#include <GLFW/glfw3.h>

#include "globals.h"

ObjLoader::ObjLoader(std::string objFilePath, std::string mtlFilePath)
{
    m_objFilePath = objFilePath;
    m_mtlFilePath = mtlFilePath;

    LoadMtlFile();
	LoadObjFile();

    for (const auto& part : ModelParts) {
        GLuint VAO, VBO;
        SetupModelPartBuffers(part, VAO, VBO);
        VAOS.push_back(VAO);
        VBOS.push_back(VBO);
    }
}

void ObjLoader::LoadMtlFile() {
    std::unordered_map<std::string, Material> materials;
    std::ifstream file(m_mtlFilePath);

    if (!file.is_open()) {
        std::cerr << "Failed to open MTL file: " << m_mtlFilePath << std::endl;
        MaterialData = materials;
        return;
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

    MaterialData = materials;
    return;
}

void ObjLoader::LoadObjFile() {
    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices, temp_normals;
    std::vector<glm::vec2> temp_uvs;
    std::string currentMaterialName;
    ModelPart* currentModelPart = nullptr;

    FILE* file = nullptr;
    errno_t err = fopen_s(&file, m_objFilePath.c_str(), "r");
    std::unique_ptr<FILE, decltype(&fclose)> file_ptr(file, &fclose);
    if (err != 0 || !file_ptr) {
        std::cerr << "The file '" << m_objFilePath << "' could not be opened\n";
        return;
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
                return;
            }
            if (currentModelPart == nullptr) {
                ModelParts.emplace_back();
                currentModelPart = &ModelParts.back();
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
            ModelParts.emplace_back();
            currentModelPart = &ModelParts.back();
            currentModelPart->materialName = currentMaterialName;
        }
    }

    return;
}

void ObjLoader::LoadTexture(int& width, int& height, int& nrChannels, unsigned int& texture, const char* file, bool hasAlpha)
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

void ObjLoader::SetupModelPartBuffers(const ModelPart& part, GLuint& VAO, GLuint& VBO) {
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