#pragma once

#include <string>
#include <vector>
#include "face.h"

struct ModelPart {
    unsigned int vertexCount;
    std::string materialName;
    std::vector<Face> faces;
};