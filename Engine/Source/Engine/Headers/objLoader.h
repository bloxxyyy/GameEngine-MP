#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "shaderHelper.h"

#include "modelPart.h"
#include "material.h"

class ObjLoader
{

private:
	std::string m_mtlFilePath;
	std::string m_objFilePath;

public:
	std::vector<GLuint> VAOS;
	std::vector<GLuint> VBOS;
	std::unordered_map<std::string, Material> MaterialData;
	std::vector<ModelPart> ModelParts;

public:
	ObjLoader(std::string objFilePath, std::string mtlFilePath);

private:
	void LoadObjFile();
	void LoadMtlFile();
	void LoadTexture(int& width, int& height, int& nrChannels, unsigned int& texture, const char* file, bool hasAlpha);
	void SetupModelPartBuffers(const ModelPart& part, GLuint& VAO, GLuint& VBO);
};