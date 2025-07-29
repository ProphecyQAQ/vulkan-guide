#include "camera.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

void Camera::update(float deltaTime)
{
    glm::mat4 cameraRotation = get_rotation_matrix();
    position += glm::vec3(cameraRotation * glm::vec4(velocity * deltaTime * 5.f, 0.f));
}

void Camera::process_SDL_event(SDL_Event& e)
{
    if (e.type == SDL_KEYDOWN) {
        if (e.key.keysym.sym == SDLK_w) { velocity.z = -1; }
        if (e.key.keysym.sym == SDLK_s) { velocity.z = 1; }
        if (e.key.keysym.sym == SDLK_a) { velocity.x = -1; }
        if (e.key.keysym.sym == SDLK_d) { velocity.x = 1; }
    }

    if (e.type == SDL_KEYUP) {
        if (e.key.keysym.sym == SDLK_w) { velocity.z = 0; }
        if (e.key.keysym.sym == SDLK_s) { velocity.z = 0; }
        if (e.key.keysym.sym == SDLK_a) { velocity.x = 0; }
        if (e.key.keysym.sym == SDLK_d) { velocity.x = 0; }
    }

    if (e.type == SDL_MOUSEMOTION)
    {
        yaw += (float)e.motion.xrel / 200.f;
        pitch -= (float)e.motion.yrel / 200.f;
    }
}

glm::mat4 Camera::get_view_matrix()
{
    glm::mat4 translation = glm::translate(position);
    glm::mat4 rotation = get_rotation_matrix();

    return glm::inverse(translation * rotation);
}

glm::mat4 Camera::get_rotation_matrix()
{
    glm::quat pitchRotation = glm::angleAxis(pitch, glm::vec3 { 1.f, 0.f, 0.f });
    glm::quat yawRotation = glm::angleAxis(yaw, glm::vec3 { 0.f, -1.f, 0.f });

    return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}