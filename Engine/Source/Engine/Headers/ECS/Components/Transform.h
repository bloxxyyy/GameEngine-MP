#pragma	once

#include "../Component.h"
#include <glm.hpp>

#include "../../camera.h"

class Transform : Component {

public:
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	Transform(glm::vec3 pos = glm::vec3(0.0f), glm::vec3 rot = glm::vec3(0.0f), glm::vec3 sca = glm::vec3(1.0f));
	glm::mat4 GetModelMatrix();
};