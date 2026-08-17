#include <data.hpp>
#include <organism.hpp>
#include <plant.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace data {
    Census performCensus(entt::registry& registry) {
        Census results = {
            .organismCount = 0,
            .plantCount = 0,
        };

        auto metabolizers = registry.view<Metabolism>();

        for (auto& candidate : metabolizers) {
            if (registry.all_of<Genome>(candidate)) {
                ++results.organismCount;
            } else {
                ++results.plantCount;
            }
        }

        return results;
    }

    PhaseRecord generateRecord(entt::registry& registry) {
        auto view = registry.view<Genome, Metabolism>();

        size_t count = 0;
        double speedSum = 0.0;
        double senseRadiusSum = 0.0;
        double sizeSum = 0.0;
        double energySum = 0.0;
        double maxEnergySum = 0.0;

        for (auto [entity, genome, metabolism] : view.each()) {
            speedSum += genome.speed;
            senseRadiusSum += genome.sense;
            sizeSum += genome.size;
            energySum += metabolism.energy;
            maxEnergySum += organism::energyLimit(genome);

            ++count;
        }

        return PhaseRecord{
            .count = count,
            .speed = speedSum / count,
            .senseRadius = senseRadiusSum / count,
            .size = sizeSum / count,
            .energy = energySum / count,
            .maxEnergy = maxEnergySum / count,
        };
    }

    void publishCurrentState(entt::registry& registry, const char* title) {
        auto view = registry.view<Genome, Metabolism>();

        std::ofstream file(title);

        file << "speed,sense,size,energy,maxEnergy\n";

        for (auto [entity, genome, metabolism] : view.each()) {
            file << genome.speed << ",";
            file << genome.sense << ",";
            file << genome.size << ",";
            file << metabolism.energy << ",";
            file << organism::energyLimit(genome) << "\n";
        }

        file.close();
    }

    void publishRecord(const PhaseRecord& record, const char* title) {
        bool exists = std::filesystem::exists(title);
        std::ofstream file(title, std::ios::app);

        if (!file.is_open()) {
            std::cerr << "file failed to open\n";

            std::exit(1);
        }

        if (!exists) {
            file << "population,avgSpeed,avgSenseRadius,avgSize,avgEnergy,avgMaxEnergy\n";
        }

        if (record.count > 0) {
            file << record.count << ",";
            file << record.speed << ",";
            file << record.senseRadius << ",";
            file << record.size << ",";
            file << record.energy << ",";
            file << record.maxEnergy << "\n";
        } else {
            file << "0,0,0,0,0,0" << "\n";
        }

        file.close();
    }
}
