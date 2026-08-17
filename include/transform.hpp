#pragma once

#include <entt/entt.hpp>
#include <glm/glm.hpp>

struct Position {
    glm::dvec2 position;
};

struct Velocity {
    glm::dvec2 velocity;
};

namespace transform::systems {
    void integrate(entt::registry& registry, double deltaTime);
}
