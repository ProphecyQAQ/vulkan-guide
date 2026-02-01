#pragma once

#include <Core/Log.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <Scene/Object.h>

class Scene
{
public:
    Scene();
    virtual ~Scene();

    void addObject(Object* object, glm::mat4 transform = glm::mat4(1.0f));

    void OnUpdate();    
private:
    std::unordered_map<Object*, glm::mat4> objects;
};