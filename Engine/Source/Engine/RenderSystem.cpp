#include "Headers/ECS/Systems/RenderSystem.h"
#include "objLoader.h"

RenderSystem::RenderSystem(std::map<int, std::shared_ptr<Transform>>& transforms, std::map<int, std::shared_ptr<MeshRenderer>>& meshRenderers)
	: transforms(transforms), meshRenderers(meshRenderers) {}

void RenderSystem::RemoveRenderable(int entityId)
{
	auto meshRenderer = meshRenderers[entityId];

	for (auto vao : meshRenderer->VAOS) {
		glDeleteVertexArrays(1, &vao);
	}

	for (auto vbo : meshRenderer->VBOS) {
		glDeleteBuffers(1, &vbo);
	}

	meshRenderer->VAOS.clear();
	meshRenderer->VBOS.clear();
	meshRenderer->ModelParts.clear();
	meshRenderer->MaterialData.clear();
	meshRenderers.erase(entityId);
}

void RenderSystem::AddNewRenderable(int entityId, std::string objFilePath, std::string mtlFilePath)
{
	ObjLoader objLoader(objFilePath, mtlFilePath);
	auto meshRenderer = std::make_shared<MeshRenderer>(entityId);
	meshRenderer->VAOS = objLoader.VAOS;
	meshRenderer->VBOS = objLoader.VBOS;
	meshRenderer->ModelParts = objLoader.ModelParts;
	meshRenderer->MaterialData = objLoader.MaterialData;

	meshRenderers[entityId] = meshRenderer;
}
