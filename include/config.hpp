#pragma once

#include <cstddef>

namespace config {
    // the size of the world in unspecified units
    constexpr static double WorldSize = 30.0;

    // the duration of a phase in seconds
    constexpr static double PhaseDuration = 15.0;

    // the number of plants spawned into the world at any given point
    constexpr static size_t PlantCount = 100;

    // the most energy a plant can provide
    constexpr static double PlantMaxEnergy = 1000.0;

    // the initial number of organisms initially spawned
    constexpr static size_t OrganismInitialCount = 50;

    // how much of an organism's volume (size^3) contributes to energy storage
    constexpr static double OrganismEnergyStorageFactor = 1.0;

    // the initial energy% of any organism (+/-10%)
    constexpr static double OrganismInitialEnergy = 1.0;

    // the initial speed of a new organism (+/-10%)
    constexpr static double OrganismInitialSpeed = 5.0;

    // the initial sense radius of a new organism (+/-10%)
    constexpr static double OrganismInitialSenseRadius = 5.0;

    // the initial size of a new organism (+/-10%)
    constexpr static double OrganismInitialSize = 10.0;

    // the energy% required to reproduce
    constexpr static double OrganismReproducerThreshold = 0.5;

    // the cooldown time for reproduction in seconds (+/-10%)
    constexpr static double OrganismReproducerCooldown = 5.0;

    // the cooldown time for wandering in seconds (+/-10%)
    constexpr static double OrganismWandererCooldown = 1.0;

    // the chance a mutation occurs when the genome is modified
    constexpr static double OrganismMutationChance = 0.5;

    // the scale of a given mutation
    constexpr static double OrganismMutationScale = 0.2;

    // the efficiency of all organisms
    constexpr static double OrganismEfficiency = 0.0005;
}
