#include <config.hpp>
#include <random.hpp>
#include <transform.hpp>

#include <numbers>

namespace transform {
    glm::dvec2 randomHeading(RandomEngine& engine) {
        double heading = engine.uniformSample(-1.0, 1.0) * std::numbers::pi * 2.0;

        return {std::cos(heading), std::sin(heading)};
    }
}

namespace transform::systems {
    void integrate(entt::registry& registry, double deltaTime) {
        auto view = registry.view<Position, Velocity>();

        for (auto [entity, position, velocity] : view.each()) {
            position.position += velocity.velocity * deltaTime;
            position.position = glm::clamp(position.position, -config::WorldSize, config::WorldSize);
        }
    }
}
