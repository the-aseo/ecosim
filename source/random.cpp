#include <random.hpp>

RandomEngine::RandomEngine()
    : engine_(device_()) {
}

double RandomEngine::uniformSample(double min, double max) {
    return std::uniform_real_distribution<double>(min, max)(engine_);
}
