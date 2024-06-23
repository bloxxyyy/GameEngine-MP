#pragma once

#include <vector>
#include <map>

#include "../Components/Transform.h"
#include "../Components/MeshRenderer.h"

class RenderSystem {

public:
    std::map<int, std::shared_ptr<Transform>>& transforms;
    std::map<int, std::shared_ptr<MeshRenderer>>& meshRenderers;

public:
    RenderSystem(std::map<int, std::shared_ptr<Transform>>& transforms, std::map<int, std::shared_ptr<MeshRenderer>>& meshRenderers);
    void AddNewRenderable(int entityId, std::string objFilePath, std::string mtlFilePath);
    void RemoveRenderable(int entityId);
};