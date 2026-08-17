#include <config.hpp>
#include <random.hpp>
#include <transform.hpp>

namespace transform::systems {
    void integrate(entt::registry& registry, double deltaTime) {
        auto view = registry.view<Position, Velocity>();

        for (auto [entity, position, velocity] : view.each()) {
            position.position += velocity.velocity * deltaTime;
            position.position = glm::clamp(position.position, -config::WorldSize, config::WorldSize);
        }
    }

}
