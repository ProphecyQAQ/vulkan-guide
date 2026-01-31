#pragma once

#include <Core/Log.h>
#include <Core/BaseType.h>
#include <filesystem>

struct BoundingBox
{
    BoundingBox() = default;
    BoundingBox(glm::vec3 minPos, glm::vec3 maxPos)
        : minPos(minPos), maxPos(maxPos) {};

    glm::vec3 minPos;
    glm::vec3 maxPos;
};

class Object
{
public:
    static Object loadGLTF(std::filesystem::path path);

    Object() = default;
    virtual ~Object() = default;

    BoundingBox& getBoundingBox() { return boundingBox; }
    std::vector<uint32_t>& getIndices() { return indices; }
    std::vector<Vertex>& getVertices() { return vertices; }
private: 
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    BoundingBox boundingBox;
};