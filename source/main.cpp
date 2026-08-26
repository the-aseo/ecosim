#include <animal.hpp>
#include <data.hpp>
#include <plant.hpp>

#include <raylib.h>

#include <filesystem>

int main() {
    if (std::filesystem::exists("plants") && std::filesystem::is_directory("plants")) {
        std::filesystem::remove_all("plants");
    }

    std::filesystem::create_directory("plants");
    std::filesystem::create_directory("plants/phases");

    if (std::filesystem::exists("animals") && std::filesystem::is_directory("animals")) {
        std::filesystem::remove_all("animals");
    }

    std::filesystem::create_directory("animals");
    std::filesystem::create_directory("animals/phases");

    InitWindow(1000, 1000, "ecosim");
    SetTargetFPS(10000);

    RandomEngine engine;
    entt::registry registry;

    for (size_t i = 0; i < 50; ++i) {
        PlantGenome plantGenome = {
            .size = 10.0 * engine.uniformSample(0.9, 1.1),
            .synthesizingArea = 0.1 * engine.uniformSample(0.9, 1.1),
            .offspringCount = 1.0 * engine.uniformSample(0.9, 1.1),
            .offspringQuality = 0.8 * engine.uniformSample(0.9, 1.1),
        };

        glm::dvec2 position = {
            engine.uniformSample(-1.0, 1.0) * config::WorldSize,
            engine.uniformSample(-1.0, 1.0) * config::WorldSize,
        };

        plant::spawn({position}, plantGenome, engine, registry);
    }

    for (size_t i = 0; i < 30; ++i) {
        AnimalGenome animalGenome = {
            .speed = 3.0 * engine.uniformSample(0.9, 1.1),
            .senseRadius = 20.0 * engine.uniformSample(0.9, 1.1),
            .size = 8.0 * engine.uniformSample(0.9, 1.1),
            .offspringCount = 1.0 * engine.uniformSample(0.9, 1.1),
            .offspringQuality = 0.75 * engine.uniformSample(0.9, 1.1),
        };

        glm::dvec2 position = {
            engine.uniformSample(-1.0, 1.0) * config::WorldSize,
            engine.uniformSample(-1.0, 1.0) * config::WorldSize,
        };

        animal::spawn({position}, animalGenome, engine, registry);
    }

    size_t phase = 0;

    Environment environment = {
        .time = 0.0,
    };

    while (!WindowShouldClose()) {
        double deltaTime = 0.01;

        plant::systems::photosynthesize(engine, registry, deltaTime);
        plant::systems::replicate(engine, registry, environment, deltaTime);

        animal::systems::replicate(engine, registry, deltaTime);
        animal::systems::move(engine, registry, deltaTime);
        animal::systems::metabolize(engine, registry, deltaTime);

        transform::systems::integrate(registry, deltaTime);

        environment.time += deltaTime;

        if (environment.time >= config::PhaseDuration) {
            environment.time = 0.0;

            auto plantRecord = data::generatePlantRecord(registry);
            auto animalRecord = data::generateAnimalRecord(registry);

            std::string currentPlantStateTitle = "plants/phases/phase" + std::to_string(phase) + ".csv";
            std::string currentAnimalStateTitle = "animals/phases/phase" + std::to_string(phase) + ".csv";
            std::string currentPlantRecordTitle = "plants/mean.csv";
            std::string currentAnimalRecordTitle = "animals/mean.csv";

            data::publishCurrentPlantStates(registry, currentPlantStateTitle.c_str());
            data::publishCurrentAnimalStates(registry, currentAnimalStateTitle.c_str());

            data::appendPlantRecord(plantRecord, currentPlantRecordTitle.c_str(), phase);
            data::appendAnimalRecord(animalRecord, currentAnimalRecordTitle.c_str(), phase);

            ++phase;
        }

        BeginDrawing();
        ClearBackground(BLACK);

        float factor = 1000.0 / config::WorldSize;

        auto plantView = registry.view<PlantGenome, Position, Metabolism>();
        auto animalView = registry.view<AnimalGenome, Position, Metabolism>();

        for (auto [plant, genome, position, metabolism] : plantView.each()) {
            Vector2 centre = {
                .x = ((static_cast<float>(factor * position.position.x) + 1000.0f) / 2.0f),
                .y = ((static_cast<float>(factor * position.position.y) + 1000.0f) / 2.0f),
            };

            double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);
            double fullPercent = metabolism.energy / genome.energyLimit(maturity);

            DrawCircleV(centre, genome.size * (factor / 2.0) * 0.1 * fullPercent, GREEN);
        }

        for (auto [animal, genome, position, metabolism] : animalView.each()) {
            Vector2 centre = {
                .x = ((static_cast<float>(factor * position.position.x) + 1000.0f) / 2.0f),
                .y = ((static_cast<float>(factor * position.position.y) + 1000.0f) / 2.0f),
            };

            double maturity = std::clamp(metabolism.age / config::JuvenileMaturityAge, 0.0, 1.0);
            double fullPercent = metabolism.energy / genome.energyLimit(maturity);

            DrawCircleV(centre, genome.size * (factor / 2.0) * 0.1 * fullPercent, RED);
            DrawCircleV(centre, genome.senseRadius * (factor / 2.0), Fade(BLUE, 0.1));
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
