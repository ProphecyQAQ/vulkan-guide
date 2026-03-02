#include <Core/Camera.h>
#include <Core/InputSystem.h>

#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

void Camera::processInput()
{
    const InputSystem& inputSystem = InputSystem::get();

    if (inputSystem.keyDown(KeyCode::W))
    {
        velocity.z = -1.f;
    }
    if (inputSystem.keyDown(KeyCode::S))
    {
        velocity.z = 1.f;
    }
    if (inputSystem.keyDown(KeyCode::A))
    {
        velocity.x = -1.f;
    }
    if (inputSystem.keyDown(KeyCode::D))
    {
        velocity.x = 1.f;
    }

    if (inputSystem.keyReleased(KeyCode::W))
    {
        velocity.z = 0.f;
    }
    if (inputSystem.keyReleased(KeyCode::S))
    {
        velocity.z = 0.f;
    }
    if (inputSystem.keyReleased(KeyCode::A))
    {
        velocity.x = 0.f;
    }
    if (inputSystem.keyReleased(KeyCode::D))
    {
        velocity.x = 0.f;
    }

    std::pair<int, int> mouseDelta = inputSystem.mouseDeltaPosition();
    yaw += static_cast<float>(mouseDelta.first) / 200.f;
    pitch -= static_cast<float>(mouseDelta.second) / 200.f;
}

void Camera::update(float deltaTime)
{
    glm::mat4 cameraRotation = getRotationMatrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * deltaTime * 5.f, 0.f));
}

glm::mat4 Camera::getViewMatrix()
{
    glm::mat4 translation = glm::translate(position);
    glm::mat4 rotation = getRotationMatrix();

    return glm::inverse(translation * rotation);
}

glm::mat4 Camera::getRotationMatrix()
{
    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3 { 1.f, 0.f, 0.f });
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3 { 0.f, -1.f, 0.f });

    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}