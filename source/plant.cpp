#include <config.hpp>
#include <organism.hpp>
#include <plant.hpp>
#include <random.hpp>
#include <transform.hpp>

#include <iostream>

namespace plant {
    entt::entity spawn(RandomEngine& engine, entt::registry& registry) {
        auto entity = registry.create();

        // create the plant's data
        auto& metabolism = registry.emplace<Metabolism>(entity);
        auto& position = registry.emplace<Position>(entity);

        metabolism.energy = config::PlantMaxEnergy * engine.uniformSample(2.0 / 3.0, 1.0);

        position.position.x = config::WorldSize * engine.uniformSample(-1.0, 1.0);
        position.position.y = config::WorldSize * engine.uniformSample(-1.0, 1.0);

        std::cout << "new plant spawned\n";

        return entity;
    }
}
