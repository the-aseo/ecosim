#pragma once

#include <random>

class RandomEngine {
public:
    RandomEngine();

    [[nodiscard]] double uniformSample(double min, double max);

private:
    std::random_device device_;
    std::mt19937 engine_;
};
