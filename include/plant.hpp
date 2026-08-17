#pragma once

#include <entt/entt.hpp>

class RandomEngine;

namespace plant {
    entt::entity spawn(RandomEngine& engine, entt::registry& registry);
}
