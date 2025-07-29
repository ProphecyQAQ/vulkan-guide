
#include <vk_types.h>
#include <SDL_events.h>

class Camera {
public:
    glm::vec3 velocity;
    glm::vec3 position;

    // vertical rotation
    float pitch { 0.f };
    // horizontal rotation
    float yaw { 0.f };

    glm::mat4 get_view_matrix();
    glm::mat4 get_rotation_matrix();

    void process_SDL_event(SDL_Event& event);

    void update();
};
