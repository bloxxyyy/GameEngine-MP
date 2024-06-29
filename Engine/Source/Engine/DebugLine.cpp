#include "Engine/Headers/Debug/DebugLine.h"
#include <camera.h>
#include <globals.h>
#include <shaderHelper.h>


DebugLine::DebugLine(glm::vec3 start, glm::vec3 end) : start(start), end(end)
{
	glGenBuffers(1, &VBO);
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	auto size = 2 * sizeof(glm::vec3);
	auto data = std::vector<glm::vec3>{ start, end };
	glBufferData(GL_ARRAY_BUFFER, size, data.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	glEnableVertexAttribArray(0);
	glBindVertexArray(0);

	std::unique_ptr<Shader> s = std::make_unique<Shader>("../Engine/Source/Engine/lineShader.vs", "../Engine/Source/Engine/lineShader.fs");
	shader = std::move(s);
}

void DebugLine::Render(Camera camera, glm::mat4 modelMatrix)
{
    shader->use();
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
    shader->setMat4("model", model);
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINES, 0, 2);
}