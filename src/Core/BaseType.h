#pragma once

#include <glm/glm.hpp>
#include <vector>

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

struct RenderData
{
    glm::mat4 transform;
    std::vector<Vertex>* vertices;
    std::vector<uint32_t>* indices;
};