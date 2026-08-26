#include <animal.hpp>
#include <data.hpp>
#include <plant.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

data::PlantPhaseRecord data::generatePlantRecord(entt::registry& registry) {
    auto view = registry.view<PlantGenome, Metabolism>();

    uint32_t count = 0;

    double sizeSum = 0.0;
    double synthesizingAreaSum = 0.0;
    double offspringCountSum = 0.0;
    double offspringQualitySum = 0.0;
    double energySum = 0.0;
    double maxEnergySum = 0.0;

    for (auto [entity, genome, metabolism] : view.each()) {
        double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);

        sizeSum += genome.size;
        synthesizingAreaSum += genome.synthesizingArea;
        offspringCountSum += genome.offspringCount;
        offspringQualitySum += genome.offspringQuality;
        energySum += metabolism.energy;
        maxEnergySum += genome.energyLimit(maturity);
        ++count;
    }

    return {
        .count = count,
        .size = sizeSum / count,
        .synthesizingArea = synthesizingAreaSum / count,
        .offspringCount = offspringCountSum / count,
        .offspringQuality = offspringQualitySum / count,
        .energy = energySum / count,
        .maxEnergy = maxEnergySum / count,
    };
}

data::AnimalPhaseRecord data::generateAnimalRecord(entt::registry& registry) {
    auto view = registry.view<AnimalGenome, Metabolism>();

    uint32_t count = 0;

    double speedSum = 0.0;
    double senseRadiusSum = 0.0;
    double sizeSum = 0.0;
    double offspringCountSum = 0.0;
    double offspringQualitySum = 0.0;
    double energySum = 0.0;
    double maxEnergySum = 0.0;

    for (auto [entity, genome, metabolism] : view.each()) {
        double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);

        speedSum += genome.speed * maturity;
        senseRadiusSum += genome.senseRadius * maturity;
        sizeSum += genome.size * maturity;
        offspringCountSum += genome.offspringCount * maturity;
        offspringQualitySum += genome.offspringQuality * maturity;
        energySum += metabolism.energy;
        maxEnergySum += genome.energyLimit(maturity);

        ++count;
    }

    return {
        .count = count,
        .speed = speedSum / count,
        .senseRadius = senseRadiusSum / count,
        .size = sizeSum / count,
        .offspringCount = offspringCountSum / count,
        .offspringQuality = offspringQualitySum / count,
        .energy = energySum / count,
        .maxEnergy = maxEnergySum / count,
    };
}

void data::publishCurrentPlantStates(entt::registry& registry, const char* title) {
    auto view = registry.view<PlantGenome, Metabolism>();

    std::ofstream file(title);

    file << "id,size,synthesizingArea,offspringCount,offspringQuality,energy,maxEnergy\n";

    for (auto [entity, genome, metabolism] : view.each()) {
        double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);

        file << entt::to_entity(entity) << ",";
        file << genome.size << ",";
        file << genome.synthesizingArea << ",";
        file << genome.offspringCount << ",";
        file << genome.offspringQuality << ",";
        file << metabolism.energy << ",";
        file << genome.energyLimit(maturity) << "\n";
    }

    file.close();
}

void data::publishCurrentAnimalStates(entt::registry& registry, const char* title) {
    auto view = registry.view<AnimalGenome, Metabolism>();

    std::ofstream file(title);

    file << "id,speed,senseRadius,size,offspringCount,offspringQuality,energy,maxEnergy\n";

    for (auto [entity, genome, metabolism] : view.each()) {
        double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);

        file << entt::to_entity(entity) << ",";
        file << genome.speed * maturity << ",";
        file << genome.senseRadius * maturity << ",";
        file << genome.size * maturity << ",";
        file << genome.offspringCount * maturity << ",";
        file << genome.offspringQuality * maturity << ",";
        file << metabolism.energy << ",";
        file << genome.energyLimit(maturity) << "\n";
    }

    file.close();
}

void data::appendPlantRecord(const PlantPhaseRecord& record, const char* title, uint32_t iteration) {
    bool exists = std::filesystem::exists(title);
    std::ofstream file(title, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "file failed to open\n";

        std::exit(1);
    }

    if (!exists) {
        file << "iteration,count,size,synthesizingArea,offspringCount,offspringQuality,energy,maxEnergy\n";
    }

    file << iteration << ",";
    file << record.count << ",";
    file << record.size << ",";
    file << record.synthesizingArea << ",";
    file << record.offspringCount << ",";
    file << record.offspringQuality << ",";
    file << record.energy << ",";
    file << record.maxEnergy << "\n";

    file.close();
}

void data::appendAnimalRecord(const AnimalPhaseRecord& record, const char* title, uint32_t iteration) {
    bool exists = std::filesystem::exists(title);
    std::ofstream file(title, std::ios::app);

    if (!file.is_open()) {
        std::cerr << "file failed to open\n";

        std::exit(1);
    }

    if (!exists) {
        file << "iteration,count,speed,senseRadius,size,offspringCount,offspringQuality,energy,maxEnergy\n";
    }

    file << iteration << ",";
    file << record.count << ",";
    file << record.speed << ",";
    file << record.senseRadius << ",";
    file << record.size << ",";
    file << record.offspringCount << ",";
    file << record.offspringQuality << ",";
    file << record.energy << ",";
    file << record.maxEnergy << "\n";

    file.close();
}
