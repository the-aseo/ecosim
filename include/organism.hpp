#pragma once

#include <entt/entt.hpp>

struct Velocity;
class RandomEngine;

struct Genome {
    double speed;
    double sense;
    double size;
};

struct Wanderer {
    double cooldown;

    [[nodiscard]] static Wanderer reset(RandomEngine& engine);
};

struct Reproducer {
    double cooldown;

    [[nodiscard]] static Reproducer reset(RandomEngine& engine);
};

struct Metabolism {
    double energy;
};

namespace organism {
    // provides the most energy an organism can store
    [[nodiscard]] double energyLimit(const Genome& genome);

    // the spent energy this cycle with respect to time
    [[nodiscard]] double energyCost(const Genome& genome, double deltaTime);

    // provide a mutated version of a genome
    [[nodiscard]] Genome mutate(const Genome& initial, RandomEngine& engine);

    // create a new heading for an organism
    [[nodiscard]] Velocity newHeading(Genome& genome, RandomEngine& engine);

    // spawn a brand new organism
    entt::entity spawn(RandomEngine& engine, entt::registry& registry);

    // derive an organism from another (reproduction, costs energy)
    entt::entity derive(RandomEngine& engine, entt::registry& registry, entt::entity parent);
}

namespace organism::systems {
    // cause organisms to wander around in space and interact with their environment
    void wander(RandomEngine& engine, entt::registry& registry, double deltaTime);

    // spend the organisms' energy for this cycle
    void metabolize(entt::registry& registry, double deltaTime);

    // cause all capable organisms to reproduce
    void reproduce(RandomEngine& engine, entt::registry& registry, double deltaTime);

    // kill all eligible organisms
    void purge(entt::registry& registry);
}
