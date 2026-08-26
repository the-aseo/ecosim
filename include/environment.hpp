#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/noise.hpp>

#include <config.hpp>
#include <random.hpp>

#include <numbers>

struct Environment {
    double time;

    [[nodiscard]] glm::dvec2 sampleWind(RandomEngine& randomEngine) const {
        double angleOffset = randomEngine.uniformSample(-1.0, 1.0);
        double speedFactor = randomEngine.uniformSample(0.5, 1.5);

        double angle = 2.0 * std::numbers::pi * glm::simplex(glm::dvec2{time, config::WindOffset});

        return config::Windspeed * speedFactor * glm::dvec2{std::cos(angle + angleOffset), std::sin(angle + angleOffset)};
    }
};
