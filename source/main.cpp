#include <config.hpp>
#include <data.hpp>
#include <organism.hpp>
#include <plant.hpp>
#include <random.hpp>
#include <transform.hpp>

#include <filesystem>
#include <iostream>

// TODO: make food a resource dependent on the environment's soil nutrient content

int main() {
    if (std::filesystem::exists("csv") && std::filesystem::is_directory("csv")) {
        std::filesystem::remove_all("csv");
    }

    std::filesystem::create_directory("csv");
    std::filesystem::create_directory("csv/phases");

    RandomEngine engine;
    entt::registry registry;

    for (size_t i = 0; i < config::PlantCount; ++i) {
        plant::spawn(engine, registry);
    }

    for (size_t i = 0; i < config::OrganismInitialCount; ++i) {
        organism::spawn(engine, registry);
    }

    using Clock = std::chrono::steady_clock;

    Clock::time_point lastTime = Clock::now();

    double timer = 0.0;
    size_t phase = 0;

    while (true) {
        Clock::time_point currentTime = Clock::now();
        std::chrono::duration<double> elapsed = currentTime - lastTime;
        double deltaTime = elapsed.count();

        lastTime = currentTime;

        organism::systems::wander(engine, registry, deltaTime);
        organism::systems::metabolize(registry, deltaTime);
        organism::systems::purge(registry);
        organism::systems::reproduce(engine, registry, deltaTime);

        transform::systems::integrate(registry, deltaTime);

        timer += deltaTime;

        while (timer >= config::PhaseDuration) {
            std::cout << "================ phase completed\n";

            timer -= config::PhaseDuration;

            data::Census census = data::performCensus(registry);
            data::PhaseRecord record = data::generateRecord(registry);

            std::string currentStateTitle = "csv/phases/phase" + std::to_string(phase) + ".csv";
            std::string currentRecordTitle = "csv/mean.csv";

            data::publishCurrentState(registry, currentStateTitle.c_str());
            data::publishRecord(record, currentRecordTitle.c_str());

            if (census.organismCount == 0) {
                return 0;
            }

            lastTime = Clock::now();
            ++phase;
        }
    }

    return 0;
}
