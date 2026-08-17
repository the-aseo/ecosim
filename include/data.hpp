#pragma once

#include <entt/entt.hpp>

namespace data {
    struct PhaseRecord {
        size_t count;
        double speed;
        double senseRadius;
        double size;
        double energy;
        double maxEnergy;
    };

    struct Census {
        size_t organismCount;
        size_t plantCount;
    };

    // collects population counts for food and organisms
    [[nodiscard]] Census performCensus(entt::registry& registry);

    // generate a new phase record
    [[nodiscard]] PhaseRecord generateRecord(entt::registry& registry);

    // publishes the current state to a csv file
    void publishCurrentState(entt::registry& registry, const char* title);

    // publishes phase record to a csv file
    void publishRecord(const PhaseRecord& record, const char* title);
}
