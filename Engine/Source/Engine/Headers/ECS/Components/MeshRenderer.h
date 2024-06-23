#pragma once

#include "../Component.h"

#include <glad/glad.h>
#include <vector>
#include <memory>
#include <unordered_map>

#include "../Components/Transform.h"
#include "../Components/MeshRenderer.h"
#include "../../modelPart.h"
#include "../../camera.h"
#include "../../shaderHelper.h"
#include "../../material.h"

class MeshRenderer : Component {

public:
    std::vector<GLuint> VAOS;
    std::vector<GLuint> VBOS;
    std::unordered_map<std::string, Material> MaterialData;
    std::vector<ModelPart> ModelParts;

private:
    std::unique_ptr<Shader> shader;

public:
    MeshRenderer(int entityId);
    void SetShader();
    void Render(Camera camera, glm::mat4 modelMatrix);

private:
    void RenderModelPart(const ModelPart& part, GLuint VAO);

};
