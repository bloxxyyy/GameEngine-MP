#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "shaderHelper.h"

#include "modelPart.h"
#include "material.h"
#include "camera.h"

class ObjLoader
{

private:
	std::string m_mtlFilePath;
	std::string m_objFilePath;
	std::unordered_map<std::string, Material> m_materials;
	std::vector<ModelPart> m_modelParts;

public:
	std::vector<GLuint> VAOS;
	std::vector<GLuint> VBOS;
	std::unique_ptr<Shader> shader;

public:
	ObjLoader(std::string objFilePath, std::string mtlFilePath);
	~ObjLoader();
	void SetShader(std::unique_ptr<Shader> newShader);
	void Render(Camera camera);

private:
	void LoadObjFile();
	void LoadMtlFile();
	void LoadTexture(int& width, int& height, int& nrChannels, unsigned int& texture, const char* file, bool hasAlpha);
	void SetupModelPartBuffers(const ModelPart& part, GLuint& VAO, GLuint& VBO);
	void RenderModelPart(const ModelPart& part, GLuint VAO);
};