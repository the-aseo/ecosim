#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

class RandomEngine;

struct Position {
    glm::dvec2 position;
};

struct Velocity {
    glm::dvec2 velocity;
};

namespace transform {
    [[nodiscard]] glm::dvec2 randomHeading(RandomEngine& engine);
}

namespace transform::systems {
    void integrate(entt::registry& registry, double deltaTime);
}
