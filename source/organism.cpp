#include <config.hpp>
#include <organism.hpp>
#include <random.hpp>
#include <transform.hpp>

#include <iostream>

Wanderer Wanderer::reset(RandomEngine& engine) {
    return {
        .cooldown = config::OrganismWandererCooldown * engine.uniformSample(0.9, 1.1),
    };
}

Reproducer Reproducer::reset(RandomEngine& engine) {
    return {
        .cooldown = config::OrganismReproducerCooldown * engine.uniformSample(0.9, 1.1),
    };
}

void populate(entt::registry& registry, entt::entity entity) {
    // this organism has a genome
    registry.emplace<Genome>(entity);

    // this organism can metabolize
    registry.emplace<Metabolism>(entity);

    // this organism has a position in the world
    registry.emplace<Position>(entity);

    // this organism can move in the world
    registry.emplace<Velocity>(entity);

    // this organism can wander around
    registry.emplace<Wanderer>(entity);

    // this organism can reproduce
    registry.emplace<Reproducer>(entity);
}

namespace organism {
    double energyLimit(const Genome& genome) {
        return std::pow(genome.size, 3.0) * config::OrganismEnergyStorageFactor;
    }

    double energyCost(Genome& genome, double deltaTime) {
        return (std::pow(genome.size, 3.0) * std::pow(genome.speed, 2.0) + genome.sense) * config::OrganismEfficiency * deltaTime;
    }

    Genome mutate(const Genome& initial, RandomEngine& engine) {
        auto mutationFactor = [](RandomEngine& engine) {
            double sample = engine.uniformSample(0.0, 1.0);

            // if the 'dice roll' is within a threshold (e.g. a chance of 0.33 is 33%)
            if (sample <= config::OrganismMutationChance) {
                return engine.uniformSample(-1.0, 1.0) * config::OrganismMutationScale;
            }

            // no mutation occurs
            return 0.0;
        };

        return {
            .speed = initial.speed + mutationFactor(engine),
            .sense = initial.sense + mutationFactor(engine),
            .size = initial.size + mutationFactor(engine),
        };
    }

    Velocity newHeading(Genome& genome, RandomEngine& engine) {
        double heading = engine.uniformSample(-M_PI, M_PI);

        return {
            .velocity = glm::dvec2{std::cos(heading), std::cos(heading)} * genome.speed,
        };
    }

    void resetCooldowns(Wanderer& wanderer, Reproducer& reproducer, RandomEngine& engine) {
        wanderer.cooldown = config::OrganismWandererCooldown * engine.uniformSample(0.5, 1.5);
        reproducer.cooldown = config::OrganismReproducerCooldown * engine.uniformSample(0.5, 1.5);
    }

    entt::entity spawn(RandomEngine& engine, entt::registry& registry) {
        auto entity = registry.create();

        // add all required components to the entity
        populate(registry, entity);

        auto& genome = registry.get<Genome>(entity);
        auto& metabolism = registry.get<Metabolism>(entity);
        auto& position = registry.get<Position>(entity);
        auto& velocity = registry.get<Velocity>(entity);
        auto& wanderer = registry.get<Wanderer>(entity);
        auto& reproducer = registry.get<Reproducer>(entity);

        // random values for later setup
        double energyPercent = config::OrganismInitialEnergy * engine.uniformSample(0.9, 1.1);

        // setup the organism's genome
        genome.speed = config::OrganismInitialSpeed * engine.uniformSample(0.9, 1.1);
        genome.sense = config::OrganismInitialSenseRadius * engine.uniformSample(0.9, 1.1);
        genome.size = config::OrganismInitialSize * engine.uniformSample(0.9, 1.1);

        // give the organism an initial energy budget
        metabolism.energy = energyPercent * energyLimit(genome);

        // place the organism somewhere in space
        position.position.x = config::WorldSize * engine.uniformSample(-1.0, 1.0);
        position.position.y = config::WorldSize * engine.uniformSample(-1.0, 1.0);

        velocity = organism::newHeading(genome, engine);

        reproducer = Reproducer::reset(engine);
        wanderer = Wanderer::reset(engine);

        std::cout << "new organism spawned\n";

        return entity;
    }

    entt::entity derive(RandomEngine& engine, entt::registry& registry, entt::entity parent) {
        auto child = registry.create();

        // collect the parent's data
        auto& parentMetabolism = registry.get<Metabolism>(parent);
        auto& parentGenome = registry.get<Genome>(parent);
        auto& parentPosition = registry.get<Position>(parent);
        auto& parentReproducer = registry.get<Reproducer>(parent);

        // add all required components to the child entity
        populate(registry, child);

        // collect the child's data
        auto& genome = registry.get<Genome>(child);
        auto& metabolism = registry.get<Metabolism>(child);
        auto& position = registry.get<Position>(child);
        auto& velocity = registry.get<Velocity>(child);
        auto& wanderer = registry.get<Wanderer>(child);
        auto& reproducer = registry.get<Reproducer>(child);

        // mutate the parent's genome
        genome = mutate(parentGenome, engine);

        // spent the parent's energy
        // TODO: make the cost factor a config option
        parentMetabolism.energy -= config::OrganismReproducerThreshold * 0.75;

        // child inheritance
        position = parentPosition;

        // give the organism an initial energy budget (what the parent spent)
        metabolism.energy = config::OrganismReproducerThreshold * 0.75;

        velocity = organism::newHeading(genome, engine);

        // reset the parent's reproduction timer
        parentReproducer = Reproducer::reset(engine);

        // set the new organism's timers
        reproducer = Reproducer::reset(engine);
        wanderer = Wanderer::reset(engine);

        std::cout << "new child birthed\n";

        return child;
    }
}

namespace organism::systems {
    void wander(RandomEngine& engine, entt::registry& registry, double deltaTime) {
        auto organismView = registry.view<Position, Velocity, Genome, Metabolism, Wanderer>();
        auto consumableView = registry.view<Position, Metabolism>();

        std::vector<entt::entity> queue;

        for (auto [organism, position, velocity, genome, metabolism, wanderer] : organismView.each()) {
            double closestConsumableDistance = std::numeric_limits<double>::max();
            double closestThreatDistance = std::numeric_limits<double>::max();

            entt::entity closestConsumable = entt::null;
            entt::entity closestThreat = entt::null;

            // find the closest food or threat
            for (auto [candidate, consumablePosition, consumableMetabolism] : consumableView.each()) {
                if (candidate == organism) {
                    continue;
                }

                double distance = glm::distance(position.position, consumablePosition.position);
                bool candidateIsOrganism = registry.all_of<Genome>(candidate);
                bool isThreat = false;
                bool isEdible = true;

                // the candidate is an organism, check if it is a threat or edible
                if (candidateIsOrganism) {
                    auto& foodGenome = registry.get<Genome>(candidate);

                    // perform the checks
                    isThreat = foodGenome.size > genome.size * 1.25;
                    isEdible = genome.size > foodGenome.size;
                }

                // the candidate can be eaten
                if (isEdible && distance <= genome.sense && distance < closestConsumableDistance) {
                    closestConsumable = candidate;
                    closestConsumableDistance = distance;
                }

                // the candidate is threatening and should be avoided
                if (isThreat && distance <= genome.sense && distance < closestThreatDistance) {
                    closestThreat = candidate;
                    closestThreatDistance = distance;
                }
            }

            bool willFlee = closestThreat != entt::null;
            bool willEat = closestConsumable != entt::null;

            if (willFlee) {
                // avoid a threat
                auto& threatPosition = registry.get<Position>(closestThreat);
                velocity.velocity = glm::normalize(position.position - threatPosition.position) * genome.speed;

                std::cout << "organism fleeing\n";
            } else if (willEat) {
                auto& foodPosition = registry.get<Position>(closestConsumable);

                // TODO: make consumption distance a config option
                if (closestConsumableDistance <= 0.1) {
                    auto& consumableMetabolism = registry.get<Metabolism>(closestConsumable);

                    // null-case protection for if it has already been eaten and should be ignored
                    if (consumableMetabolism.energy == 0.0) {
                        continue;
                    }

                    metabolism.energy += consumableMetabolism.energy;
                    metabolism.energy = std::clamp(metabolism.energy, 0.0, energyLimit(genome));

                    // protect against double-consume cases where 2+ organisms reach the food in the same tick
                    consumableMetabolism.energy = 0.0;

                    queue.emplace_back(closestConsumable);

                    std::cout << "organism ate something\n";

                } else {
                    auto direction = glm::normalize(foodPosition.position - position.position);
                    velocity.velocity = direction * genome.speed;
                }
            } else {
                wanderer.cooldown -= deltaTime;

                if (wanderer.cooldown <= 0.0) {
                    wanderer = Wanderer::reset(engine);
                    velocity = newHeading(genome, engine);
                }
            }
        }

        for (auto& consumable : queue) {
            registry.destroy(consumable);
        }
    }

    void reproduce(RandomEngine& engine, entt::registry& registry, double deltaTime) {
        auto view = registry.view<Reproducer, Metabolism, Genome>();

        std::vector<entt::entity> queue;

        for (auto [entity, reproduction, metabolism, genome] : view.each()) {
            double energyNeeded = organism::energyLimit(genome) * config::OrganismReproducerThreshold;

            bool cooldownComplete = reproduction.cooldown == 0.0;
            bool enoughEnergy = metabolism.energy >= energyNeeded;

            if (cooldownComplete && enoughEnergy) {
                queue.emplace_back(entity);
            }

            // ensure the cooldown is non-negative
            reproduction.cooldown = std::max(0.0, reproduction.cooldown - deltaTime);
        }

        for (auto& entity : queue) {
            organism::derive(engine, registry, entity);
        }
    }

    void metabolize(entt::registry& registry, double deltaTime) {
        auto view = registry.view<Metabolism, Genome>();

        for (auto [entity, metabolism, genome] : view.each()) {
            metabolism.energy -= energyCost(genome, deltaTime);
            metabolism.energy = std::clamp(metabolism.energy, 0.0, energyLimit(genome));
        }
    }

    void purge(entt::registry& registry) {
        auto view = registry.view<Genome, Metabolism>();

        std::vector<entt::entity> queue;

        for (auto [entity, genome, metabolism] : view.each()) {
            if (metabolism.energy == 0.0) {
                queue.emplace_back(entity);

                std::cout << "organism killed\n";
            }
        }

        for (auto& entity : queue) {
            registry.destroy(entity);
        }
    }
}
