#pragma once

#include <entt/entt.hpp>

#include <common.hpp>
#include <config.hpp>
#include <environment.hpp>
#include <random.hpp>
#include <transform.hpp>

struct PlantGenome {
    double size;
    double synthesizingArea;

    double offspringCount;
    double offspringQuality;

    [[nodiscard]] double energyLimit(double maturity) const {
        return std::pow(size * maturity, 3.0) * (1.0 - synthesizingArea);
    }

    [[nodiscard]] double energyGainPerSecond() const {
        return std::pow(size, 3.0) * synthesizingArea;
    }

    [[nodiscard]] double energyCostPerSecond() const {
        double synthesizingCost = std::pow(std::pow(size, 3.0) * synthesizingArea, 0.5);
        double sizeCost = std::pow(std::pow(size, 3.0) * synthesizingArea, 2.0 / 3.0);

        return synthesizingCost + sizeCost;
    }

    [[nodiscard]] uint32_t rollOffspringCount(RandomEngine& randomEngine) const {
        double value = randomEngine.uniformSample(0.5, 1.5);

        return std::floor(value * offspringCount);
    }

    [[nodiscard]] double energyCostPerOffspring(double maturity) {
        return offspringQuality * energyLimit(maturity) * config::ReproductionCostFactor;
    }

    [[nodiscard]] double gestationPeriod() {
        return offspringQuality * std::pow(offspringCount, 0.5) * config::PlantGestationPeriodFactor;
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

    [[nodiscard]] static PlantGenome mutate(const PlantGenome& genome, RandomEngine& randomEngine) {
        auto mutationFactor = [&]() {
            double mutationAmount = 1.0 + (randomEngine.uniformSample(-1.0, 1.0) * config::MutationFactor);
            double roll = randomEngine.uniformSample(0.0, 1.0);

            return roll <= config::MutationChance ? mutationAmount : 1.0;
        };

        double newSize = genome.size * mutationFactor();
        double newOffspringCount = genome.offspringCount * mutationFactor();
        double newSynthesizingArea = genome.synthesizingArea * mutationFactor();
        double newOffspringQuality = genome.offspringQuality * mutationFactor();

        return {
            .size = newSize,
            .synthesizingArea = std::clamp(newSynthesizingArea, 0.0, 1.0),
            .offspringCount = newOffspringCount,
            .offspringQuality = std::clamp(newOffspringQuality, 0.0, 1.0),
        };
    }
};

namespace plant {
    inline void spawn(const Position& position, const PlantGenome& genome, RandomEngine& randomEngine, entt::registry& registry) {
        auto plant = registry.create();

        double startingAge = genome.offspringQuality * config::JuvenileMaturityAge;
        double maturity = std::clamp(startingAge / config::JuvenileMaturityAge, 0.0, 1.0);

        registry.emplace<PlantGenome>(plant, genome);
        registry.emplace<Metabolism>(plant, Metabolism{.energy = 0.5 * genome.energyLimit(maturity), .age = startingAge});
        registry.emplace<Position>(plant, position);
    }
}

namespace plant::systems {
    inline void replicate(RandomEngine& randomEngine, entt::registry& registry, Environment& environment, double deltaTime) {
        auto plantView = registry.view<PlantGenome, Metabolism, Position>();

        struct GestationCache {
            uint32_t offspringCount;
            entt::entity entity;
        };

        std::vector<GestationCache> gestationQueue;
        std::vector<entt::entity> birthQueue;

        for (auto [plant, genome, metabolism, position] : plantView.each()) {
            double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);

            if (maturity < 0.8) {
                continue;
            }

            bool gestating = registry.all_of<Gestation>(plant);

            if (gestating) {
                auto& gestationTimer = registry.get<Gestation>(plant);

                if (gestationTimer.time >= genome.gestationPeriod() * maturity) {
                    birthQueue.emplace_back(plant);
                } else {
                    gestationTimer.time += deltaTime;
                }

                continue;
            }

            uint32_t count = genome.rollOffspringCount(randomEngine);
            double energyRequired = count * genome.energyCostPerOffspring(maturity);

            if (energyRequired < genome.energyLimit(maturity)) {
                metabolism.energy -= energyRequired;

                gestationQueue.emplace_back(count, plant);
            }
        }

        for (auto plant : gestationQueue) {
            registry.emplace<Gestation>(plant.entity, Gestation{0.0, plant.offspringCount});
        }

        for (auto plant : birthQueue) {
            uint32_t count = registry.get<Gestation>(plant).offspringCount;
            auto& position = registry.get<Position>(plant).position;
            auto& genome = registry.get<PlantGenome>(plant);

            for (uint32_t i = 0; i < count; ++i) {
                auto wind = environment.sampleWind(randomEngine);
                glm::dvec2 newPosition = position + wind;

                newPosition = glm::clamp(newPosition, glm::dvec2{-config::WorldSize}, glm::dvec2{config::WorldSize});

                spawn({newPosition}, PlantGenome::mutate(genome, randomEngine), randomEngine, registry);
            }

            registry.remove<Gestation>(plant);
        }
    }

    inline void photosynthesize(RandomEngine& randomEngine, entt::registry& registry, double deltaTime) {
        auto plantView = registry.view<PlantGenome, Metabolism>();

        std::vector<entt::entity> deathQueue;

        for (auto [plant, genome, metabolism] : plantView.each()) {
            double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);
            double juvenileMultiplier = 1.0 + config::JuvenilePenaltyFactor * (1.0 - maturity);

            double gain = genome.energyGainPerSecond() * deltaTime;
            double loss = genome.energyCostPerSecond() * juvenileMultiplier * deltaTime;

            metabolism.energy += gain;
            metabolism.energy -= loss;
            metabolism.energy = std::clamp(metabolism.energy, 0.0, genome.energyLimit(maturity));
            metabolism.age += deltaTime;

            double hazardRate = genome.senescenceHazardRate(metabolism.age);
            double deathChance = 1.0 - std::exp(-hazardRate * deltaTime);

            bool diedOfAge = randomEngine.uniformSample(0.0, 1.0) < deathChance;
            bool diedOfEnergy = metabolism.energy <= 0.0;

            if (diedOfAge || diedOfEnergy) {
                deathQueue.emplace_back(plant);
            }
        }

        for (auto& plant : deathQueue) {
            registry.destroy(plant);
        }
    }
}
