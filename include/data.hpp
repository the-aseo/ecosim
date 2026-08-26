#pragma once

#include <entt/entt.hpp>

namespace data {
    struct PlantPhaseRecord {
        size_t count;
        double size;
        double synthesizingArea;
        double offspringCount;
        double offspringQuality;
        double energy;
        double maxEnergy;
    };

    struct AnimalPhaseRecord {
        size_t count;
        double speed;
        double senseRadius;
        double size;
        double offspringCount;
        double offspringQuality;
        double energy;
        double maxEnergy;
    };

    [[nodiscard]] PlantPhaseRecord generatePlantRecord(entt::registry& registry);
    [[nodiscard]] AnimalPhaseRecord generateAnimalRecord(entt::registry& registry);

    void publishCurrentPlantStates(entt::registry& registry, const char* title);
    void publishCurrentAnimalStates(entt::registry& registry, const char* title);

    void appendPlantRecord(const PlantPhaseRecord& record, const char* title, uint32_t iteration);
    void appendAnimalRecord(const AnimalPhaseRecord& record, const char* title, uint32_t iteration);
}
