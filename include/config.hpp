#pragma once

#include <cstddef>

namespace config {
    constexpr static double WorldSize = 100.0;
    constexpr static double WindOffset = 0.0;
    constexpr static double Windspeed = 5.0;
    constexpr static double PhaseDuration = 15.0;

    constexpr static double MutationChance = 0.25;
    constexpr static double MutationFactor = 0.5;

    constexpr static double ReproductionCostFactor = 0.5;
    constexpr static double PlantGestationPeriodFactor = 80.0;
    constexpr static double AnimalGestationPeriodFactor = 30.0;

    constexpr static double JuvenileMaturityAge = 20.0;
    constexpr static double JuvenilePenaltyFactor = 0.5;

    constexpr static double SenescenceOnsetAgeFactor = 30.0;
    constexpr static double SenescenceRiskFactor = 0.05;

    constexpr static double MetabolicCostFactor = 0.025;
    constexpr static double SenseCostFactor = 0.04;
    constexpr static double SpeedCostFactor = 0.04;

    constexpr static double WanderTurnRate = 0.15;
    constexpr static double HungryThreshold = 0.85;
}
