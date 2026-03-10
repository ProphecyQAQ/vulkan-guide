#pragma once

#include <glm/glm.hpp>

class Camera
{
public:
    Camera() = default;
    ~Camera() = default;

    void update(float deltaTime);
    void processInput();
    
    glm::mat4 getViewMatrix();
    glm::mat4 getRotationMatrix();
private:
    glm::vec3 velocity{0.f};
    glm::vec3 position{0.f, 0.f, 5.f};

    // vertical rotation
    float pitch { 0.f };
    // horizontal rotation
    float yaw { 0.f };
};