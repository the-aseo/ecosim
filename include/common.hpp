#pragma once

#include <entt/entt.hpp>

struct Metabolism {
    double energy;
    double age;
};

struct Gestation {
    double time;
    uint32_t offspringCount;
};

struct Lineage {
    uint32_t generation;
    entt::entity parent;
};
