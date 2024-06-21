#pragma once

#include <string>
#include <glm.hpp>

struct Material {
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
    std::string diffuseMap;
    std::string diffuseTexture;
    unsigned int textureID;
};
