#pragma once

#include <glm/glm.hpp>

struct Vertex
{
    Vertex() 
        : position(0.0f), uv_u(0.0f), normal(0.0f), uv_v(0.0f), color(1.0f){}

    glm::vec3 position;
    float uv_u;
    glm::vec3 normal;
    float uv_v;
    glm::vec3 color;
};