#pragma once

#include <entt/entt.hpp>

#include <common.hpp>
#include <config.hpp>
#include <environment.hpp>
#include <plant.hpp>
#include <random.hpp>
#include <transform.hpp>

struct Velocity;
struct Environment;
class RandomEngine;

struct AnimalGenome {
    double speed;
    double senseRadius;
    double size;

    double offspringCount;
    double offspringQuality;

    [[nodiscard]] double energyLimit(double maturity) const {
        return std::pow(size * maturity, 3.0);
    }

    [[nodiscard]] double energyCostPerSecond() const {
        double metabolicCost = std::pow(size, 3.0) * config::MetabolicCostFactor;
        double senseCost = senseRadius * config::SenseCostFactor;
        double speedCost = std::pow(speed, 2.0) * config::SpeedCostFactor;

        return metabolicCost + senseCost + speedCost;
    }

    [[nodiscard]] uint32_t rollOffspringCount(RandomEngine& randomEngine) const {
        double value = randomEngine.uniformSample(0.5, 1.5);

        return std::floor(value * offspringCount);
    }

    [[nodiscard]] double energyCostPerOffspring(double maturity) {
        return offspringQuality * energyLimit(maturity) * config::ReproductionCostFactor;
    }

    [[nodiscard]] double gestationPeriod() {
        return offspringQuality * std::pow(offspringCount, 0.5) * config::AnimalGestationPeriodFactor;
    }

    [[nodiscard]] double senescenceOnsetAge() const {
        return size * config::SenescenceOnsetAgeFactor;
    }

    [[nodiscard]] double senescenceHazardRate(double age) const {
        double onset = senescenceOnsetAge();
        if (age <= onset)
            return 0.0;

        double overshoot = age - onset;
        return config::SenescenceRiskFactor * overshoot;
    }

    [[nodiscard]] static AnimalGenome mutate(const AnimalGenome& genome, RandomEngine& randomEngine) {
        auto mutationFactor = [&]() {
            double mutationAmount = 1.0 + (randomEngine.uniformSample(-1.0, 1.0) * config::MutationFactor);
            double roll = randomEngine.uniformSample(0.0, 1.0);

            return roll <= config::MutationChance ? mutationAmount : 1.0;
        };

        double newSize = genome.size * mutationFactor();
        double newSenseRadius = genome.senseRadius * mutationFactor();
        double newSpeed = genome.speed * mutationFactor();
        double newOffspringCount = genome.offspringCount * mutationFactor();
        double newOffspringQuality = genome.offspringQuality * mutationFactor();

        return {
            .speed = newSpeed,
            .senseRadius = newSenseRadius,
            .size = newSize,
            .offspringCount = newOffspringCount,
            .offspringQuality = std::clamp(newOffspringQuality, 0.0, 1.0),
        };
    }
};

namespace animal {
    inline void spawn(const Position& position, const AnimalGenome& genome, RandomEngine& engine, entt::registry& registry, entt::entity parent = entt::null) {
        auto animal = registry.create();

        double startingAge = genome.offspringQuality * config::JuvenileMaturityAge;
        double maturity = std::clamp(startingAge / config::JuvenileMaturityAge, 0.0, 1.0);

        registry.emplace<AnimalGenome>(animal, genome);
        registry.emplace<Metabolism>(animal, Metabolism{.energy = genome.energyLimit(maturity), .age = startingAge});
        registry.emplace<Position>(animal, position);
        registry.emplace<Velocity>(animal, transform::randomHeading(engine) * genome.speed);

        auto& lineage = registry.emplace<Lineage>(animal);

        if (registry.valid(parent)) {
            lineage.generation = registry.get<Lineage>(parent).generation + 1;
            lineage.parent = parent;
        } else {
            lineage.generation = 0;
            lineage.parent = entt::null;
        }
    }
}

namespace animal::systems {
    inline void replicate(RandomEngine& randomEngine, entt::registry& registry, double deltaTime) {
        auto animalView = registry.view<AnimalGenome, Metabolism, Position>();

        struct GestationCache {
            uint32_t offspringCount;
            entt::entity entity;
        };

        std::vector<GestationCache> gestationQueue;
        std::vector<entt::entity> birthQueue;

        for (auto [animal, genome, metabolism, position] : animalView.each()) {
            double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);

            if (maturity < 0.9) {
                // too young to reproduce
                continue;
            }

            bool gestating = registry.all_of<Gestation>(animal);

            if (gestating) {
                auto& gestationTimer = registry.get<Gestation>(animal);

                if (gestationTimer.time >= genome.gestationPeriod() * maturity) {
                    birthQueue.emplace_back(animal);
                } else {
                    gestationTimer.time += deltaTime;
                }

                continue;
            }

            uint32_t count = genome.rollOffspringCount(randomEngine);
            double energyRequired = count * genome.energyCostPerOffspring(maturity);

            if (energyRequired < genome.energyLimit(maturity)) {
                metabolism.energy -= energyRequired;

                gestationQueue.emplace_back(count, animal);
            }
        }

        for (auto plant : gestationQueue) {
            registry.emplace<Gestation>(plant.entity, Gestation{0.0, plant.offspringCount});
        }

        for (auto animal : birthQueue) {
            uint32_t count = registry.get<Gestation>(animal).offspringCount;
            auto& position = registry.get<Position>(animal).position;
            auto& genome = registry.get<AnimalGenome>(animal);
            auto& lineage = registry.get<Lineage>(animal);

            for (uint32_t i = 0; i < count; ++i) {
                spawn({position}, AnimalGenome::mutate(genome, randomEngine), randomEngine, registry, animal);
            }

            registry.remove<Gestation>(animal);
        }
    }

    inline void move(RandomEngine& randomEngine, entt::registry& registry, double deltaTime) {
        auto animalView = registry.view<AnimalGenome, Position, Velocity, Metabolism, Lineage>();
        auto plantView = registry.view<PlantGenome, Position>();

        enum class Action {
            Wander,
            Hunt,
            Flee,
        };

        struct ConsumptionCache {
            entt::entity consumed;
            entt::entity consumer;
        };

        std::vector<ConsumptionCache> consumptionList;

        for (auto [animal, genome, position, velocity, metabolism, lineage] : animalView.each()) {
            Action action = Action::Wander;

            double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);
            double closestPreyDistance = std::numeric_limits<double>::max();
            double cumulativeThreatDistance = 1.0;

            glm::dvec2 moveDirection = {0, 0};

            entt::entity potentialPrey = entt::null;
            bool hungry = (genome.energyLimit(maturity) * config::HungryThreshold) >= metabolism.energy;

            for (auto [plant, otherGenome, otherPosition] : plantView.each()) {
                double distance = glm::distance(position.position, otherPosition.position);

                // cannot see the plant
                if (distance > genome.senseRadius * maturity || distance == 0.0) {
                    continue;
                }

                if (action != Action::Flee && distance < closestPreyDistance && hungry) {
                    closestPreyDistance = distance;
                    moveDirection = otherPosition.position - position.position;
                    action = Action::Hunt;
                    potentialPrey = plant;
                }
            }

            for (auto [otherAnimal, otherGenome, otherPosition, otherVelocity, otherMetabolism, otherLineage] : animalView.each()) {
                if (animal == otherAnimal) {
                    continue;
                }

                double otherMaturity = std::clamp(otherMetabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);
                double distance = glm::distance(position.position, otherPosition.position);

                // cannot see the animal
                if (distance > genome.senseRadius * maturity || distance == 0.0) {
                    continue;
                }

                bool parent = lineage.parent == otherAnimal;
                bool child = otherLineage.parent == animal;
                bool threatening = genome.size * maturity <= otherGenome.size * otherMaturity * 1.25;
                bool appealing = otherGenome.size * otherMaturity <= genome.size * maturity * 1.25;

                if (!parent && !child && action != Action::Flee && distance < closestPreyDistance && appealing && hungry) {
                    closestPreyDistance = distance;
                    moveDirection = otherPosition.position - position.position;
                    action = Action::Hunt;
                    potentialPrey = otherAnimal;
                } else if (!parent && !child && threatening) {
                    if (action != Action::Flee) {
                        moveDirection = {0, 0};
                    }

                    cumulativeThreatDistance += distance;
                    moveDirection += position.position - otherPosition.position;
                    action = Action::Flee;
                    potentialPrey = entt::null;
                }
            }

            if (action == Action::Flee) {
                moveDirection /= cumulativeThreatDistance;
            }

            if (action == Action::Wander) {
                glm::dvec2 currentDirection = glm::length(velocity.velocity) > 0.0
                                                  ? glm::normalize(velocity.velocity)
                                                  : transform::randomHeading(randomEngine);

                double turnAngle = randomEngine.uniformSample(-config::WanderTurnRate, config::WanderTurnRate);

                double cosA = std::cos(turnAngle);
                double sinA = std::sin(turnAngle);

                moveDirection = {
                    currentDirection.x * cosA - currentDirection.y * sinA,
                    currentDirection.x * sinA + currentDirection.y * cosA,
                };
            }

            if (glm::length(moveDirection) == 0.0) {
                moveDirection = transform::randomHeading(randomEngine);
            }

            velocity.velocity = glm::normalize(moveDirection) * genome.speed * maturity;

            if (action == Action::Hunt && closestPreyDistance <= 0.1 && potentialPrey != entt::null) {
                consumptionList.emplace_back(potentialPrey, animal);
            }
        }

        for (auto& cache : consumptionList) {
            // prey and/or consumer were already consumed
            if (!registry.valid(cache.consumed) || !registry.valid(cache.consumer)) {
                continue;
            }

            auto& consumedMetabolism = registry.get<Metabolism>(cache.consumed);
            auto& consumerMetabolism = registry.get<Metabolism>(cache.consumer);
            auto& genome = registry.get<AnimalGenome>(cache.consumer);
            auto& metabolism = registry.get<Metabolism>(cache.consumer);

            double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);
            double maxEnergy = genome.energyLimit(maturity);
            double maxEnergyDelta = maxEnergy - consumerMetabolism.energy;

            if (registry.all_of<PlantGenome>(cache.consumed)) {
                if (consumedMetabolism.energy >= maxEnergyDelta) {
                    consumedMetabolism.energy -= maxEnergyDelta;
                    consumerMetabolism.energy += maxEnergyDelta;

                    if (consumedMetabolism.energy <= 0.0) {
                        registry.destroy(cache.consumed);
                    }
                } else {
                    consumerMetabolism.energy += consumedMetabolism.energy;
                    consumedMetabolism.energy = 0.0;

                    registry.destroy(cache.consumed);
                }
            } else {
                consumerMetabolism.energy += std::min(maxEnergyDelta, consumedMetabolism.energy);
                consumedMetabolism.energy = 0.0;

                registry.destroy(cache.consumed);
            }
        }
    }

    inline void metabolize(RandomEngine& randomEngine, entt::registry& registry, double deltaTime) {
        auto animalView = registry.view<AnimalGenome, Metabolism>();

        std::vector<entt::entity> deathQueue;

        for (auto [animal, genome, metabolism] : animalView.each()) {
            double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);
            double juvenileMultiplier = 1.0 + config::JuvenilePenaltyFactor * (1.0 - maturity);

            double loss = genome.energyCostPerSecond() * juvenileMultiplier * deltaTime;

            metabolism.energy -= loss;
            metabolism.energy = std::clamp(metabolism.energy, 0.0, genome.energyLimit(maturity));
            metabolism.age += deltaTime;

            double hazardRate = genome.senescenceHazardRate(metabolism.age);
            double deathChance = 1.0 - std::exp(-hazardRate * deltaTime);

            bool diedOfAge = randomEngine.uniformSample(0.0, 1.0) < deathChance;
            bool diedOfEnergy = metabolism.energy <= 0.0;

            if (diedOfAge || diedOfEnergy) {
                deathQueue.emplace_back(animal);
            }
        }

        for (auto& plant : deathQueue) {
            registry.destroy(plant);
        }
    }
}
