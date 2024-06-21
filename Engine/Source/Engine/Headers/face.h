#pragma once

#include <glm.hpp>

struct Face {
    glm::vec3 vertices[3];
    glm::vec2 uvs[3];
    glm::vec3 normals[3];
};