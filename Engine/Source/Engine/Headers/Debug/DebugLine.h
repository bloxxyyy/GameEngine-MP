#pragma once
#include <glad/glad.h>
#include <memory>

#include "../camera.h"
#include "../globals.h"
#include "../shaderHelper.h"


class DebugLine
{

private:
	GLuint VBO;
	GLuint VAO;
	std::unique_ptr<Shader> shader;
	glm::vec3 start;
	glm::vec3 end;

public:
	DebugLine(glm::vec3 start, glm::vec3 end);
	void Render(Camera camera, glm::mat4 modelMatrix);
};