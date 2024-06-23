#include "Headers/ECS/Components/MeshRenderer.h"

#include <GLFW/glfw3.h>

#include "material.h"
#include "globals.h"
#include "shaderHelper.h"

MeshRenderer::MeshRenderer(int entityId) : shader(nullptr)
{
    EntityID = entityId;
    SetShader();
}

void MeshRenderer::SetShader()
{
    std::unique_ptr<Shader> s = std::make_unique<Shader>("../Engine/Source/Engine/modelShader.vs", "../Engine/Source/Engine/modelShader.fs");
    shader = std::move(s);
}

void MeshRenderer::Render(Camera camera, glm::mat4 modelMatrix) {

    if (!shader) {
        std::cerr << "Shader is null!" << std::endl;
        return;
    }

    shader->use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 model = modelMatrix;
    model = glm::rotate(model, glm::radians(50.0f) * (float)glfwGetTime(), glm::vec3(0.5f, 1.0f, 0.0f));

    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);

    int index = 0;
    for (const auto& part : ModelParts) {
        RenderModelPart(part, VAOS[index]);
        index++;
    }
}

void MeshRenderer::RenderModelPart(const ModelPart& part, GLuint vao) {
    glBindVertexArray(vao);

    auto it = MaterialData.find(part.materialName);
    if (it != MaterialData.end()) {
        const Material& material = it->second;
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material.textureID);
        shader->setInt("texture1", 0);
    }

    glDrawArrays(GL_TRIANGLES, 0, part.vertexCount);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
}
