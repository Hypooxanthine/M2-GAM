#pragma once

#include <glm/glm.hpp>

namespace vrm
{

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
    float scalar;
};

} // namespace vrm