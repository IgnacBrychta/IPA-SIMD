#include "VolcanoSim.h"
#include "Terrain.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>

#include <cstdlib>

namespace {

struct VolcanoParticleSIMDParams {
    float dt;
    float gravity;
    float plumeBuoyancy;
    float ambientTemperature;
    float damping;
    float windX;
    float windZ;
    float particleCooling;
};

extern "C" float volcanoFlowFromDropAsm(float drop, float k);
extern "C" void volcanoParticleStepAsm(
    float* posX, float* posY, float* posZ,
    float* velX, float* velY, float* velZ,
    float dt, float gravity, float plumeBuoyancy, float tempNorm,
    float damping, float windX, float windZ, float invR, float ventPush);
extern "C" void volcanoParticleStepSIMDAsm(
    float* posX, float* posY, float* posZ,
    float* velX, float* velY, float* velZ,
    float* life, float* temp,
    std::size_t count, const VolcanoParticleSIMDParams* params);
extern "C" void volcanoParticleStepBatchAsm(
    float* posX, float* posY, float* posZ,
    float* velX, float* velY, float* velZ,
    float* life, float* temp,
    std::size_t count, const VolcanoParticleSIMDParams* params);
extern "C" void volcanoUpdateLavaFluxSIMDAsm(
    float* lavaHeightNext, const float* lavaHeight, const float* terrainHeight, const float* temperature,
    int width, int height, float viscDt);
extern "C" void volcanoDiffuseHeatSIMDAsm(
    float* temperatureNext, const float* temperature, const float* lavaHeight,
    int width, int height, float alphaDt, float dt, float lavaCooling, float ambientTemperature);

static inline float max0(float v) {
    return (v > 0.0f) ? v : 0.0f;
}

static inline float flowFromDropScalar(float drop, float k) {
    const float pos = max0(drop);
    const float shape = 0.35f + 0.65f * std::sqrt(pos + 1.0e-4f);
    return pos * k * shape;
}

static inline float ventPulseAtTime(float t) {
    const float base = 0.76f + 0.19f * std::sin(t * 0.37f + 0.6f * std::sin(t * 0.11f));
    const float carrier = max0(std::sin(t * 1.45f + 0.85f * std::sin(t * 0.22f)));
    const float burst = carrier * carrier * (0.65f + 0.35f * carrier);
    return std::clamp(base + 0.52f * burst, 0.45f, 1.85f);
}

static inline float gaussian2D(float x, float y, float sigma) {
    const float inv = 1.0f / std::max(sigma, 1.0e-4f);
    const float nx = x * inv;
    const float ny = y * inv;
    return std::exp(-(nx * nx + ny * ny));
}

static float maxAbsDiff(const float* a, const float* b, std::size_t count) {
    float maxDiff = 0.0f;
    for (std::size_t i = 0; i < count; ++i) {
        maxDiff = std::max(maxDiff, std::fabs(a[i] - b[i]));
    }
    return maxDiff;
}

static std::uint64_t hashFloatArray(const float* data, std::size_t count) {
    const std::uint64_t fnvOffset = 1469598103934665603ull;
    const std::uint64_t fnvPrime = 1099511628211ull;
    std::uint64_t hash = fnvOffset;

    for (std::size_t i = 0; i < count; ++i) {
        std::uint32_t bits = 0u;
        std::memcpy(&bits, data + i, sizeof(float));
        hash ^= static_cast<std::uint64_t>(bits);
        hash *= fnvPrime;
    }

    return hash;
}

static void printReferenceIteration(const VolcanoSimulation& sim, int frame) {
    const std::size_t particleCount = sim.getParticleCount();
    const std::size_t gridCount = static_cast<std::size_t>(sim.getGridWidth()) * static_cast<std::size_t>(sim.getGridHeight());
    const float* posX = sim.getParticlePosX();
    const float* posY = sim.getParticlePosY();
    const float* posZ = sim.getParticlePosZ();
    const float* pTemp = sim.getParticleTemperature();
    const float* lava = sim.getLavaHeightGrid();
    const float* gTemp = sim.getTemperatureGrid();

    const float p0x = (particleCount > 0) ? posX[0] : 0.0f;
    const float p0y = (particleCount > 0) ? posY[0] : 0.0f;
    const float p0z = (particleCount > 0) ? posZ[0] : 0.0f;
    const float p0t = (particleCount > 0) ? pTemp[0] : 0.0f;

    std::cout << "REF frame=" << frame
              << " avgLava=" << sim.getAverageLavaHeight()
              << " avgTemp=" << sim.getAverageTemperature()
              << " p0=(" << p0x << "," << p0y << "," << p0z << ")"
              << " p0Temp=" << p0t
              << " lavaHash=" << hashFloatArray(lava, gridCount)
              << " tempHash=" << hashFloatArray(gTemp, gridCount)
              << '\n';
}

} // namespace

VolcanoSimulation::AlignedFloatArray::AlignedFloatArray() : ptr(nullptr), count(0) {}

VolcanoSimulation::AlignedFloatArray::~AlignedFloatArray() {
    if (ptr != nullptr) {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
    }
}

VolcanoSimulation::AlignedFloatArray::AlignedFloatArray(const AlignedFloatArray& other) : ptr(nullptr), count(0) {
    if (other.count > 0 && resize(other.count)) {
        std::memcpy(ptr, other.ptr, other.count * sizeof(float));
    }
}

VolcanoSimulation::AlignedFloatArray& VolcanoSimulation::AlignedFloatArray::operator=(const AlignedFloatArray& other) {
    if (this == &other) {
        return *this;
    }

    if (!resize(other.count)) {
        return *this;
    }

    if (count > 0) {
        std::memcpy(ptr, other.ptr, count * sizeof(float));
    }

    return *this;
}

VolcanoSimulation::AlignedFloatArray::AlignedFloatArray(AlignedFloatArray&& other) noexcept : ptr(other.ptr), count(other.count) {
    other.ptr = nullptr;
    other.count = 0;
}

VolcanoSimulation::AlignedFloatArray& VolcanoSimulation::AlignedFloatArray::operator=(AlignedFloatArray&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    if (ptr != nullptr) {
        std::free(ptr);
    }

    ptr = other.ptr;
    count = other.count;
    other.ptr = nullptr;
    other.count = 0;

    return *this;
}

bool VolcanoSimulation::AlignedFloatArray::resize(std::size_t newCount) {
    if (ptr != nullptr) {
        std::free(ptr);
        ptr = nullptr;
    }

    count = 0;

    if (newCount == 0) {
        return true;
    }

    void* raw = nullptr;

    #ifdef _WIN32
        raw = _aligned_malloc(newCount * sizeof(float), 32);
        if (!raw) {
            return false;
        }
    #else
        if (posix_memalign(&raw, 32, newCount * sizeof(float)) != 0) {
            return false;
        }
    #endif

    ptr = static_cast<float*>(raw);
    count = newCount;
    std::memset(ptr, 0, count * sizeof(float));
    return true;
}

float* VolcanoSimulation::AlignedFloatArray::data() {
    return ptr;
}

const float* VolcanoSimulation::AlignedFloatArray::data() const {
    return ptr;
}

std::size_t VolcanoSimulation::AlignedFloatArray::size() const {
    return count;
}

bool VolcanoSimulation::ParticleSoA::resize(std::size_t count) {
    return posX.resize(count) && posY.resize(count) && posZ.resize(count) && velX.resize(count) && velY.resize(count) &&
           velZ.resize(count) && life.resize(count) && temperature.resize(count);
}

std::size_t VolcanoSimulation::ParticleSoA::size() const {
    return posX.size();
}

VolcanoSimulation::VolcanoSimulation()
    : width(0),
      height(0),
      gravity(-9.81f),
      damping(0.992f),
      particleCooling(1.2f),
      lavaViscosity(1.18f),
      lavaCooling(0.06f),
      diffusionAlpha(0.17f),
      ambientTemperature(295.0f),
      ventTemperature(1360.0f),
      lavaSourceRate(3.4f),
      windX(0.08f),
      windZ(-0.03f),
      plumeBuoyancy(7.5f), // (plumeBuoyancy 16.0 -> 7.5)
      eruptionClock(0.0f),
      ventPulseLevel(2.0f),
      renderConfig({28.0f, 0.4f, 24.0f, 13.0f, 12.0f, 22.0f, 1.7f}),
      averageLavaHeight(0.0f),
      averageTemperature(295.0f),
      collisionTerrainSize(0),
      collisionTerrainOriginX(0.0f),
      collisionTerrainOriginZ(0.0f),
      collisionTerrainSpacing(1.0f),
      collisionWaterLevelWorld(0.0f),
      rngState(1337u) {}

VolcanoSimulation::~VolcanoSimulation() = default;

bool VolcanoSimulation::init(std::size_t particleCount, int gridWidth, int gridHeight, unsigned int seed) {
    if (gridWidth < 3 || gridHeight < 3) {
        return false;
    }

    width = gridWidth;
    height = gridHeight;
    rngState = seed;

    if (!particles.resize(particleCount)) {
        return false;
    }

    const std::size_t gridSize = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (!terrainHeight.resize(gridSize) || !lavaHeight.resize(gridSize) || !lavaHeightNext.resize(gridSize) ||
        !temperature.resize(gridSize) || !temperatureNext.resize(gridSize)) {
        return false;
    }

    for (std::size_t i = 0; i < particles.size(); ++i) {
        respawnParticle(i);
    }

    initTerrainAndLava();
    updateStats();
    return true;
}

std::size_t VolcanoSimulation::idx(int x, int y) const {
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x);
}

float VolcanoSimulation::random01() {
    rngState = 1664525u * rngState + 1013904223u;
    return static_cast<float>((rngState >> 8) & 0x00FFFFFFu) / 16777215.0f;
}

void VolcanoSimulation::setCollisionTerrain(const Terrain& terrain, float waterLevelWorld) {
    collisionTerrainSize = terrain.getGridSize();
    collisionTerrainSpacing = terrain.getGridSpacing();
    collisionWaterLevelWorld = waterLevelWorld;
    collisionTerrainHeights.clear();

    if (collisionTerrainSize <= 1 || collisionTerrainSpacing <= 0.0f) {
        collisionTerrainSize = 0;
        return;
    }

    const glm::vec3 origin = terrain.getVertex(0, 0);
    collisionTerrainOriginX = origin.x;
    collisionTerrainOriginZ = origin.z;

    const std::size_t count = static_cast<std::size_t>(collisionTerrainSize) * static_cast<std::size_t>(collisionTerrainSize);
    collisionTerrainHeights.resize(count, collisionWaterLevelWorld);

    for (int x = 0; x < collisionTerrainSize; ++x) {
        for (int z = 0; z < collisionTerrainSize; ++z) {
            const std::size_t id = static_cast<std::size_t>(x) * static_cast<std::size_t>(collisionTerrainSize) + static_cast<std::size_t>(z);
            collisionTerrainHeights[id] = terrain.getVertex(x, z).y;
        }
    }
}

float VolcanoSimulation::sampleCollisionTerrainHeightWorld(float worldX, float worldZ) const {
    if (collisionTerrainSize <= 1 || collisionTerrainHeights.empty() || collisionTerrainSpacing <= 0.0f) {
        return collisionWaterLevelWorld;
    }

    const float fx = (worldX - collisionTerrainOriginX) / collisionTerrainSpacing;
    const float fz = (worldZ - collisionTerrainOriginZ) / collisionTerrainSpacing;

    if (!std::isfinite(fx) || !std::isfinite(fz)) {
        return collisionWaterLevelWorld;
    }

    const float maxIdx = static_cast<float>(collisionTerrainSize - 1);
    const float clampedX = std::clamp(fx, 0.0f, maxIdx);
    const float clampedZ = std::clamp(fz, 0.0f, maxIdx);

    const int x0 = static_cast<int>(std::floor(clampedX));
    const int z0 = static_cast<int>(std::floor(clampedZ));
    const int x1 = std::min(x0 + 1, collisionTerrainSize - 1);
    const int z1 = std::min(z0 + 1, collisionTerrainSize - 1);

    const float tx = clampedX - static_cast<float>(x0);
    const float tz = clampedZ - static_cast<float>(z0);

    const auto at = [&](int x, int z) {
        return collisionTerrainHeights[static_cast<std::size_t>(x) * static_cast<std::size_t>(collisionTerrainSize) + static_cast<std::size_t>(z)];
    };

    const float h00 = at(x0, z0);
    const float h10 = at(x1, z0);
    const float h01 = at(x0, z1);
    const float h11 = at(x1, z1);

    const float hx0 = h00 + tx * (h10 - h00);
    const float hx1 = h01 + tx * (h11 - h01);
    return hx0 + tz * (hx1 - hx0);
}

void VolcanoSimulation::sampleCollisionTerrainGradientWorld(float worldX, float worldZ, float& dHdX, float& dHdZ) const {
    const float eps = std::max(0.25f, collisionTerrainSpacing);
    const float hx0 = sampleCollisionTerrainHeightWorld(worldX - eps, worldZ);
    const float hx1 = sampleCollisionTerrainHeightWorld(worldX + eps, worldZ);
    const float hz0 = sampleCollisionTerrainHeightWorld(worldX, worldZ - eps);
    const float hz1 = sampleCollisionTerrainHeightWorld(worldX, worldZ + eps);
    dHdX = (hx1 - hx0) / (2.0f * eps);
    dHdZ = (hz1 - hz0) / (2.0f * eps);
}

void VolcanoSimulation::respawnParticle(std::size_t i) {
    const float pulse = std::clamp(ventPulseLevel, 0.45f, 2.0f);
    const float swirl = eruptionClock * (0.42f + 0.28f * pulse);

    const int jetCount = 3;
    const int jetId = std::min(jetCount - 1, static_cast<int>(random01() * static_cast<float>(jetCount)));
    const float jetBaseAngle = swirl + (6.28318530718f * static_cast<float>(jetId) / static_cast<float>(jetCount));
    const float angle = jetBaseAngle + (random01() - 0.5f) * (0.62f - 0.16f * std::min(1.0f, pulse));
    const float radius = (0.09f + 0.22f * std::sqrt(random01())) * (0.80f + 0.26f * pulse);

    const float jitterMag = 0.06f + 0.08f * random01();
    const float jitterAngle = 6.28318530718f * random01();
    const float dx = std::cos(angle) * radius + std::cos(jitterAngle) * jitterMag;
    const float dz = std::sin(angle) * radius + std::sin(jitterAngle) * jitterMag;

    particles.posX.data()[i] = dx;
    particles.posY.data()[i] = 0.08f + random01() * (0.22f + 0.18f * pulse);
    particles.posZ.data()[i] = dz;

    const float radialSpeed = (2.2f + random01() * 3.8f) * (0.85f + 0.55f * pulse);
    const float crossBias = (random01() - 0.5f) * (1.10f + 0.65f * pulse);
    particles.velX.data()[i] = std::cos(angle) * radialSpeed + std::cos(angle + 1.57079632679f) * crossBias + windX * 0.9f;
    particles.velY.data()[i] = (8.0f + random01() * 5.5f) * (0.88f + 0.52f * pulse);
    particles.velZ.data()[i] = std::sin(angle) * radialSpeed + std::sin(angle + 1.57079632679f) * crossBias + windZ * 0.9f;

    particles.life.data()[i] = (3.2f + random01() * 2.8f) * (0.92f + 0.16f * pulse);
    particles.temperature.data()[i] = 900.0f + random01() * (340.0f + 170.0f * std::min(1.0f, pulse));
}

void VolcanoSimulation::initTerrainAndLava() {
    const int cx = width / 2;
    const int cy = height / 2;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const float nx = (2.0f * static_cast<float>(x) / static_cast<float>(width - 1)) - 1.0f;
            const float ny = (2.0f * static_cast<float>(y) / static_cast<float>(height - 1)) - 1.0f;
            const float r = std::sqrt(nx * nx + ny * ny);
            const float rClamped = std::min(1.0f, r);

            const float cone = std::exp(-2.7f * r * r);
            const float crater = 0.30f * std::exp(-220.0f * r * r);
            const float ridge = 0.03f * std::sin(11.0f * nx) * std::cos(9.0f * ny) * (1.0f - rClamped);

            const std::size_t id = idx(x, y);
            terrainHeight.data()[id] = std::max(0.0f, 0.12f + 1.1f * cone - crater + ridge);

            const float ventMask = std::exp(-190.0f * r * r);
            lavaHeight.data()[id] = 0.52f * ventMask;
            temperature.data()[id] = ambientTemperature + (ventTemperature - ambientTemperature) * ventMask;
        }
    }

    std::memcpy(lavaHeightNext.data(), lavaHeight.data(), lavaHeight.size() * sizeof(float));
    std::memcpy(temperatureNext.data(), temperature.data(), temperature.size() * sizeof(float));

    // Keep cell exactly at crater hot to avoid delayed startup.
    const std::size_t c = idx(cx, cy);
    temperature.data()[c] = ventTemperature;
    lavaHeight.data()[c] = std::max(lavaHeight.data()[c], 0.70f);
}

void VolcanoSimulation::injectVentSources(float dt) {
    eruptionClock += dt;
    const int cx = width / 2;
    const int cy = height / 2;
    const float targetPulse = ventPulseAtTime(eruptionClock);
    const float stochasticPulse = 1.0f + 0.16f * (random01() - 0.5f);
    const float desiredPulse = targetPulse * stochasticPulse;
    const float relax = std::clamp(dt * 2.2f, 0.0f, 1.0f);
    ventPulseLevel += (desiredPulse - ventPulseLevel) * relax;
    const float pulse = std::clamp(ventPulseLevel, 0.45f, 1.95f);

    const float rot = eruptionClock * 0.34f;
    const float c = std::cos(rot);
    const float s = std::sin(rot);
    const float nozzleR = 1.28f;
    const float j0x = c * nozzleR;
    const float j0y = s * nozzleR;
    const float j1x = std::cos(rot + 2.09439510239f) * nozzleR;
    const float j1y = std::sin(rot + 2.09439510239f) * nozzleR;
    const float j2x = std::cos(rot + 4.18879020479f) * nozzleR;
    const float j2y = std::sin(rot + 4.18879020479f) * nozzleR;
    const float ventTempPeak = std::min(1540.0f, ventTemperature + 80.0f * (pulse - 1.0f));

    for (int dy = -3; dy <= 3; ++dy) {
        for (int dx = -3; dx <= 3; ++dx) {
            const int gx = cx + dx;
            const int gy = cy + dy;
            if (gx < 1 || gy < 1 || gx >= width - 1 || gy >= height - 1) {
                continue;
            }

            const float fx = static_cast<float>(dx);
            const float fy = static_cast<float>(dy);
            const float core = gaussian2D(fx, fy, 1.25f);
            const float jet0 = gaussian2D(fx - j0x, fy - j0y, 0.95f);
            const float jet1 = gaussian2D(fx - j1x, fy - j1y, 0.95f);
            const float jet2 = gaussian2D(fx - j2x, fy - j2y, 0.95f);
            const float jetLobes = std::max(jet0, std::max(jet1, jet2));
            const float skirt = gaussian2D(fx, fy, 2.35f) * 0.28f;
            const float w = std::clamp(0.50f * core + 0.68f * jetLobes + skirt, 0.0f, 1.18f);
            const std::size_t id = idx(gx, gy);

            lavaHeight.data()[id] += lavaSourceRate * dt * w * (0.74f + 0.92f * pulse);
            temperature.data()[id] = std::min(1540.0f, temperature.data()[id] + (ventTempPeak - temperature.data()[id]) * w * dt * (1.1f + 0.9f * pulse));
        }
    }

    // Low frequency directional drift + turbulent wobble keeps vent ejection believable.
    const float windTargetX = 0.16f * std::sin(eruptionClock * 0.19f + 0.7f * std::sin(eruptionClock * 0.05f));
    const float windTargetZ = 0.14f * std::cos(eruptionClock * 0.17f + 0.6f * std::sin(eruptionClock * 0.07f));
    windX += (windTargetX - windX) * std::clamp(dt * 0.8f, 0.0f, 1.0f);
    windZ += (windTargetZ - windZ) * std::clamp(dt * 0.8f, 0.0f, 1.0f);
    windX = std::clamp(windX + (random01() - 0.5f) * dt * 0.14f, -0.30f, 0.30f);
    windZ = std::clamp(windZ + (random01() - 0.5f) * dt * 0.14f, -0.30f, 0.30f);
}

void VolcanoSimulation::updateStats() {
    const std::size_t n = lavaHeight.size();
    if (n == 0) {
        averageLavaHeight = 0.0f;
        averageTemperature = ambientTemperature;
        return;
    }

    float lavaSum = 0.0f;
    float tempSum = 0.0f;

    const float* lava = lavaHeight.data();
    const float* temp = temperature.data();

    for (std::size_t i = 0; i < n; ++i) {
        const float l = std::isfinite(lava[i]) ? lava[i] : 0.0f;
        const float t = std::isfinite(temp[i]) ? temp[i] : ambientTemperature;
        lavaSum += l;
        tempSum += t;
    }

    const float invN = 1.0f / static_cast<float>(n);
    averageLavaHeight = lavaSum * invN;
    averageTemperature = tempSum * invN;
}

void VolcanoSimulation::updateParticlesScalar(float dt) {
    const std::size_t n = particles.size();

    float* posX = particles.posX.data();
    float* posY = particles.posY.data();
    float* posZ = particles.posZ.data();
    float* velX = particles.velX.data();
    float* velY = particles.velY.data();
    float* velZ = particles.velZ.data();
    float* life = particles.life.data();
    float* temp = particles.temperature.data();

    const float invTempRange = 1.0f / 900.0f;
    const float emitterBaseWorldY = renderConfig.worldY + renderConfig.coneHeight + 0.25f;
    const float minLocalY = (collisionWaterLevelWorld - emitterBaseWorldY) - 2.0f;

    for (std::size_t i = 0; i < n; ++i) {
        if (!std::isfinite(posX[i]) || !std::isfinite(posY[i]) || !std::isfinite(posZ[i]) || !std::isfinite(velX[i]) ||
            !std::isfinite(velY[i]) || !std::isfinite(velZ[i]) || !std::isfinite(life[i]) || !std::isfinite(temp[i])) {
            respawnParticle(i);
            continue;
        }

        const float tempNorm = std::clamp((temp[i] - ambientTemperature) * invTempRange, 0.0f, 1.6f);
        const float radius2 = posX[i] * posX[i] + posZ[i] * posZ[i];
        const float invR = 1.0f / std::sqrt(std::max(radius2, 0.04f));
        const float ventPush = max0(1.2f - radius2) * tempNorm * 5.2f;
        volcanoParticleStepAsm(
            posX + i, posY + i, posZ + i,
            velX + i, velY + i, velZ + i,
            dt, gravity, plumeBuoyancy, tempNorm,
            damping, windX, windZ, invR, ventPush);

        life[i] -= dt * (0.14f + 0.10f * tempNorm);
        temp[i] -= particleCooling * dt * (0.55f + 0.6f * std::sqrt(radius2 + 0.1f));

        const float worldX = renderConfig.worldX + posX[i] * renderConfig.particleSpreadScale;
        const float worldZ = renderConfig.worldZ + posZ[i] * renderConfig.particleSpreadScale;
        const float terrainWorldY = sampleCollisionTerrainHeightWorld(worldX, worldZ);
        const float hitWorldY = std::max(terrainWorldY, collisionWaterLevelWorld);
        const float hitLocalY = hitWorldY - emitterBaseWorldY + 0.08f;

        if (velY[i] < 0.0f && posY[i] <= hitLocalY) {
            if (terrainWorldY > collisionWaterLevelWorld + 0.03f) {
                float dHdX = 0.0f;
                float dHdZ = 0.0f;
                sampleCollisionTerrainGradientWorld(worldX, worldZ, dHdX, dHdZ);

                float downX = -dHdX;
                float downZ = -dHdZ;
                float downLen = std::sqrt(downX * downX + downZ * downZ);
                if (downLen < 1.0e-5f) {
                    downX = posX[i];
                    downZ = posZ[i];
                    downLen = std::sqrt(downX * downX + downZ * downZ);
                }
                if (downLen < 1.0e-5f) {
                    downX = 1.0f;
                    downZ = 0.0f;
                    downLen = 1.0f;
                }

                downX /= downLen;
                downZ /= downLen;
                const float rollSpeedWorld = 1.4f + 2.6f * std::clamp((temp[i] - ambientTemperature) / 1100.0f, 0.0f, 1.0f);
                const float invSpread = 1.0f / std::max(renderConfig.particleSpreadScale, 1.0e-4f);
                velX[i] = downX * rollSpeedWorld * invSpread;
                velZ[i] = downZ * rollSpeedWorld * invSpread;
                velY[i] = 0.0f;
                posY[i] = hitLocalY + 0.03f;
            } else {
                respawnParticle(i);
            }
            continue;
        }

        if (!std::isfinite(posX[i]) || !std::isfinite(posY[i]) || !std::isfinite(posZ[i]) || !std::isfinite(velX[i]) ||
            !std::isfinite(velY[i]) || !std::isfinite(velZ[i]) || !std::isfinite(life[i]) || !std::isfinite(temp[i]) ||
            life[i] <= 0.0f || posY[i] < minLocalY || posY[i] > 64.0f) {
            respawnParticle(i);
        }
    }
}

void VolcanoSimulation::updateParticlesSIMD(float dt) {
    const std::size_t n = particles.size();

    float* posX = particles.posX.data();
    float* posY = particles.posY.data();
    float* posZ = particles.posZ.data();
    float* velX = particles.velX.data();
    float* velY = particles.velY.data();
    float* velZ = particles.velZ.data();
    float* life = particles.life.data();
    float* temp = particles.temperature.data();

    const float emitterBaseWorldY = renderConfig.worldY + renderConfig.coneHeight + 0.25f;
    const float minLocalY = (collisionWaterLevelWorld - emitterBaseWorldY) - 2.0f;

    const VolcanoParticleSIMDParams params = {
        dt,
        gravity,
        plumeBuoyancy,
        ambientTemperature,
        damping,
        windX,
        windZ,
        particleCooling
    };
    volcanoParticleStepBatchAsm(posX, posY, posZ, velX, velY, velZ, life, temp, n, &params);

    for (std::size_t i = 0; i < n; ++i) {
        const float worldX = renderConfig.worldX + posX[i] * renderConfig.particleSpreadScale;
        const float worldZ = renderConfig.worldZ + posZ[i] * renderConfig.particleSpreadScale;
        const float terrainWorldY = sampleCollisionTerrainHeightWorld(worldX, worldZ);
        const float hitWorldY = std::max(terrainWorldY, collisionWaterLevelWorld);
        const float hitLocalY = hitWorldY - emitterBaseWorldY + 0.08f;

        if (velY[i] < 0.0f && posY[i] <= hitLocalY) {
            if (terrainWorldY > collisionWaterLevelWorld + 0.03f) {
                float dHdX = 0.0f;
                float dHdZ = 0.0f;
                sampleCollisionTerrainGradientWorld(worldX, worldZ, dHdX, dHdZ);

                float downX = -dHdX;
                float downZ = -dHdZ;
                float downLen = std::sqrt(downX * downX + downZ * downZ);
                if (downLen < 1.0e-5f) {
                    downX = posX[i];
                    downZ = posZ[i];
                    downLen = std::sqrt(downX * downX + downZ * downZ);
                }
                if (downLen < 1.0e-5f) {
                    downX = 1.0f;
                    downZ = 0.0f;
                    downLen = 1.0f;
                }

                downX /= downLen;
                downZ /= downLen;
                const float rollSpeedWorld = 1.4f + 2.6f * std::clamp((temp[i] - ambientTemperature) / 1100.0f, 0.0f, 1.0f);
                const float invSpread = 1.0f / std::max(renderConfig.particleSpreadScale, 1.0e-4f);
                velX[i] = downX * rollSpeedWorld * invSpread;
                velZ[i] = downZ * rollSpeedWorld * invSpread;
                velY[i] = 0.0f;
                posY[i] = hitLocalY + 0.03f;
            } else {
                respawnParticle(i);
            }
            continue;
        }

        if (life[i] <= 0.0f || posY[i] < minLocalY || posY[i] > 64.0f) {
            respawnParticle(i);
        }
    }
}

void VolcanoSimulation::updateLavaFluxScalar(float dt) {
    std::memcpy(lavaHeightNext.data(), lavaHeight.data(), lavaHeight.size() * sizeof(float));

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            const std::size_t id = idx(x, y);
            const std::size_t l = id - 1;
            const std::size_t r = id + 1;
            const std::size_t u = id - width;
            const std::size_t d = id + width;

            const float hC = terrainHeight.data()[id] + lavaHeight.data()[id];
            const float hL = terrainHeight.data()[l] + lavaHeight.data()[l];
            const float hR = terrainHeight.data()[r] + lavaHeight.data()[r];
            const float hU = terrainHeight.data()[u] + lavaHeight.data()[u];
            const float hD = terrainHeight.data()[d] + lavaHeight.data()[d];

            const float fluidC = std::clamp((temperature.data()[id] - 650.0f) / 650.0f, 0.0f, 1.0f);
            const float fluidL = std::clamp((temperature.data()[l] - 650.0f) / 650.0f, 0.0f, 1.0f);
            const float fluidR = std::clamp((temperature.data()[r] - 650.0f) / 650.0f, 0.0f, 1.0f);
            const float fluidU = std::clamp((temperature.data()[u] - 650.0f) / 650.0f, 0.0f, 1.0f);
            const float fluidD = std::clamp((temperature.data()[d] - 650.0f) / 650.0f, 0.0f, 1.0f);

            const float kC = lavaViscosity * dt * (0.18f + 1.22f * fluidC);
            const float kL = lavaViscosity * dt * (0.18f + 1.22f * fluidL);
            const float kR = lavaViscosity * dt * (0.18f + 1.22f * fluidR);
            const float kU = lavaViscosity * dt * (0.18f + 1.22f * fluidU);
            const float kD = lavaViscosity * dt * (0.18f + 1.22f * fluidD);

            float outL = volcanoFlowFromDropAsm(hC - hL, kC);
            float outR = volcanoFlowFromDropAsm(hC - hR, kC);
            float outU = volcanoFlowFromDropAsm(hC - hU, kC);
            float outD = volcanoFlowFromDropAsm(hC - hD, kC);

            const float inL = volcanoFlowFromDropAsm(hL - hC, kL);
            const float inR = volcanoFlowFromDropAsm(hR - hC, kR);
            const float inU = volcanoFlowFromDropAsm(hU - hC, kU);
            const float inD = volcanoFlowFromDropAsm(hD - hC, kD);

            float outSum = outL + outR + outU + outD;
            const float inSum = inL + inR + inU + inD;

            const float maxOut = lavaHeight.data()[id] * 0.94f;
            if (outSum > maxOut && outSum > 1.0e-6f) {
                const float scale = maxOut / outSum;
                outL *= scale;
                outR *= scale;
                outU *= scale;
                outD *= scale;
                outSum = outL + outR + outU + outD;
            }

            const float next = std::max(0.0f, lavaHeight.data()[id] + inSum - outSum);
            lavaHeightNext.data()[id] = std::isfinite(next) ? std::min(6.0f, next) : 0.0f;
        }
    }

    std::memcpy(lavaHeight.data(), lavaHeightNext.data(), lavaHeight.size() * sizeof(float));
}

void VolcanoSimulation::updateLavaFluxSIMD(float dt) {
    std::memcpy(lavaHeightNext.data(), lavaHeight.data(), lavaHeight.size() * sizeof(float));
    const float viscDt = lavaViscosity * dt;
    volcanoUpdateLavaFluxSIMDAsm(
        lavaHeightNext.data(), lavaHeight.data(), terrainHeight.data(), temperature.data(),
        width, height, viscDt);

    const int scalarStartX = 1 + ((width - 2) / 8) * 8;

    for (int y = 1; y < height - 1; ++y) {
        const std::size_t row = static_cast<std::size_t>(y) * static_cast<std::size_t>(width);
        for (int x = scalarStartX; x < width - 1; ++x) {
            const std::size_t id = row + static_cast<std::size_t>(x);
            const std::size_t l = id - 1;
            const std::size_t r = id + 1;
            const std::size_t u = id - width;
            const std::size_t d = id + width;

            const float hC = terrainHeight.data()[id] + lavaHeight.data()[id];
            const float hL = terrainHeight.data()[l] + lavaHeight.data()[l];
            const float hR = terrainHeight.data()[r] + lavaHeight.data()[r];
            const float hU = terrainHeight.data()[u] + lavaHeight.data()[u];
            const float hD = terrainHeight.data()[d] + lavaHeight.data()[d];

            const float fluidC = std::clamp((temperature.data()[id] - 650.0f) / 650.0f, 0.0f, 1.0f);
            const float fluidL = std::clamp((temperature.data()[l] - 650.0f) / 650.0f, 0.0f, 1.0f);
            const float fluidR = std::clamp((temperature.data()[r] - 650.0f) / 650.0f, 0.0f, 1.0f);
            const float fluidU = std::clamp((temperature.data()[u] - 650.0f) / 650.0f, 0.0f, 1.0f);
            const float fluidD = std::clamp((temperature.data()[d] - 650.0f) / 650.0f, 0.0f, 1.0f);

            const float kC = viscDt * (0.18f + 1.22f * fluidC);
            const float kL = viscDt * (0.18f + 1.22f * fluidL);
            const float kR = viscDt * (0.18f + 1.22f * fluidR);
            const float kU = viscDt * (0.18f + 1.22f * fluidU);
            const float kD = viscDt * (0.18f + 1.22f * fluidD);

            float outL = volcanoFlowFromDropAsm(hC - hL, kC);
            float outR = volcanoFlowFromDropAsm(hC - hR, kC);
            float outU = volcanoFlowFromDropAsm(hC - hU, kC);
            float outD = volcanoFlowFromDropAsm(hC - hD, kC);

            const float inL = volcanoFlowFromDropAsm(hL - hC, kL);
            const float inR = volcanoFlowFromDropAsm(hR - hC, kR);
            const float inU = volcanoFlowFromDropAsm(hU - hC, kU);
            const float inD = volcanoFlowFromDropAsm(hD - hC, kD);

            float outSum = outL + outR + outU + outD;
            const float inSum = inL + inR + inU + inD;

            const float maxOut = lavaHeight.data()[id] * 0.94f;
            if (outSum > maxOut && outSum > 1.0e-6f) {
                const float scale = maxOut / outSum;
                outL *= scale;
                outR *= scale;
                outU *= scale;
                outD *= scale;
                outSum = outL + outR + outU + outD;
            }

            float next = lavaHeight.data()[id] + inSum - outSum;
            if (!std::isfinite(next)) {
                next = 0.0f;
            }
            lavaHeightNext.data()[id] = std::max(0.0f, std::min(6.0f, next));
        }
    }

    std::memcpy(lavaHeight.data(), lavaHeightNext.data(), lavaHeight.size() * sizeof(float));
}

void VolcanoSimulation::diffuseHeatScalar(float dt) {
    std::memcpy(temperatureNext.data(), temperature.data(), temperature.size() * sizeof(float));

    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            const std::size_t id = idx(x, y);
            const float c = temperature.data()[id];
            const float l = temperature.data()[id - 1];
            const float r = temperature.data()[id + 1];
            const float u = temperature.data()[id - width];
            const float d = temperature.data()[id + width];

            const float lap = l + r + u + d - 4.0f * c;
            const float fluid = std::clamp((c - 650.0f) / 650.0f, 0.0f, 1.0f);
            const float lavaHeat = lavaHeight.data()[id] * (420.0f + 100.0f * fluid);
            const float cooling = lavaCooling * (c - ambientTemperature);

            float next = c + dt * (diffusionAlpha * lap + lavaHeat - cooling);
            next = std::clamp(next, ambientTemperature, 1520.0f);
            temperatureNext.data()[id] = std::isfinite(next) ? next : ambientTemperature;
        }
    }

    const int cx = width / 2;
    const int cy = height / 2;
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            const int gx = cx + dx;
            const int gy = cy + dy;
            if (gx < 1 || gy < 1 || gx >= width - 1 || gy >= height - 1) {
                continue;
            }

            const float r2 = static_cast<float>(dx * dx + dy * dy);
            const float w = std::exp(-0.75f * r2);
            const std::size_t id = idx(gx, gy);
            const float target = ambientTemperature + (ventTemperature - ambientTemperature) * w * 0.95f;
            temperatureNext.data()[id] = std::max(temperatureNext.data()[id], target);
        }
    }

    std::memcpy(temperature.data(), temperatureNext.data(), temperature.size() * sizeof(float));
}

void VolcanoSimulation::diffuseHeatSIMD(float dt) {
    std::memcpy(temperatureNext.data(), temperature.data(), temperature.size() * sizeof(float));
    const float alphaDt = diffusionAlpha * dt;
    volcanoDiffuseHeatSIMDAsm(
        temperatureNext.data(), temperature.data(), lavaHeight.data(),
        width, height, alphaDt, dt, lavaCooling, ambientTemperature);

    const int scalarStartX = 1 + ((width - 2) / 8) * 8;

    for (int y = 1; y < height - 1; ++y) {
        const std::size_t row = static_cast<std::size_t>(y) * static_cast<std::size_t>(width);
        for (int x = scalarStartX; x < width - 1; ++x) {
            const std::size_t id = row + static_cast<std::size_t>(x);
            const float c = temperature.data()[id];
            const float l = temperature.data()[id - 1];
            const float r = temperature.data()[id + 1];
            const float u = temperature.data()[id - width];
            const float d = temperature.data()[id + width];

            const float lap = l + r + u + d - 4.0f * c;
            const float fluid = std::clamp((c - 650.0f) / 650.0f, 0.0f, 1.0f);
            const float lavaHeat = lavaHeight.data()[id] * (420.0f + 100.0f * fluid);
            const float cooling = lavaCooling * (c - ambientTemperature);

            float next = c + dt * (diffusionAlpha * lap + lavaHeat - cooling);
            temperatureNext.data()[id] = std::isfinite(next) ? std::clamp(next, ambientTemperature, 1520.0f) : ambientTemperature;
        }
    }

    const int cx = width / 2;
    const int cy = height / 2;
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            const int gx = cx + dx;
            const int gy = cy + dy;
            if (gx < 1 || gy < 1 || gx >= width - 1 || gy >= height - 1) {
                continue;
            }

            const float r2 = static_cast<float>(dx * dx + dy * dy);
            const float w = std::exp(-0.75f * r2);
            const std::size_t id = idx(gx, gy);
            const float target = ambientTemperature + (ventTemperature - ambientTemperature) * w * 0.95f;
            temperatureNext.data()[id] = std::max(temperatureNext.data()[id], target);
        }
    }

    std::memcpy(temperature.data(), temperatureNext.data(), temperature.size() * sizeof(float));
}

void VolcanoSimulation::stepScalar(float dt) {
    dt = std::clamp(dt, 1.0f / 300.0f, 0.05f);

    updateParticlesScalar(dt);
    injectVentSources(dt);
    updateLavaFluxScalar(dt);
    diffuseHeatScalar(dt);
    updateStats();
}

void VolcanoSimulation::stepSIMD(float dt) {
    dt = std::clamp(dt, 1.0f / 300.0f, 0.05f);

    updateParticlesSIMD(dt);
    injectVentSources(dt);

    updateLavaFluxSIMD(dt);
    diffuseHeatSIMD(dt);
    updateStats();
}

void runVolcanoReference(int frames) {
    const std::size_t particleCount = 65536;
    const int gridWidth = 192;
    const int gridHeight = 192;
    const float dt = 1.0f / 60.0f;

    VolcanoSimulation scalarSim;
    if (!scalarSim.init(particleCount, gridWidth, gridHeight, 1337u)) {
        std::cerr << "Volcano reference initialization failed." << std::endl;
        return;
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Volcano reference sequence (" << frames << " frames)\n";
    for (int i = 0; i < frames; ++i) {
        scalarSim.stepScalar(dt);
        printReferenceIteration(scalarSim, i + 1);
    }
}

void runVolcanoBenchmark() {
    const std::size_t particleCount = 65536;
    const int gridWidth = 192;
    const int gridHeight = 192;
    const int frames = 600;
    const int verifyFrames = 120;
    const int referenceFrames = 8;
    const float dt = 1.0f / 60.0f;

    VolcanoSimulation scalarSim;
    VolcanoSimulation simdSim;

    if (!scalarSim.init(particleCount, gridWidth, gridHeight, 1337u) ||
        !simdSim.init(particleCount, gridWidth, gridHeight, 1337u)) {
        std::cerr << "Volcano benchmark initialization failed." << std::endl;
        return;
    }

    float maxPosXDiff = 0.0f;
    float maxPosYDiff = 0.0f;
    float maxPosZDiff = 0.0f;
    float maxParticleTempDiff = 0.0f;
    float maxLavaDiff = 0.0f;
    float maxGridTempDiff = 0.0f;
    float maxAvgLavaDiff = 0.0f;
    float maxAvgTempDiff = 0.0f;

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Volcano correctness check (" << verifyFrames << " frames)\n";
    std::cout << "Reference scalar iterations for golden checks:\n";
    for (int i = 0; i < verifyFrames; ++i) {
        scalarSim.stepScalar(dt);
        simdSim.stepSIMD(dt);

        if (i < referenceFrames) {
            printReferenceIteration(scalarSim, i + 1);
        }

        const std::size_t particleN = scalarSim.getParticleCount();
        const std::size_t gridN = static_cast<std::size_t>(scalarSim.getGridWidth()) * static_cast<std::size_t>(scalarSim.getGridHeight());

        maxPosXDiff = std::max(maxPosXDiff, maxAbsDiff(scalarSim.getParticlePosX(), simdSim.getParticlePosX(), particleN));
        maxPosYDiff = std::max(maxPosYDiff, maxAbsDiff(scalarSim.getParticlePosY(), simdSim.getParticlePosY(), particleN));
        maxPosZDiff = std::max(maxPosZDiff, maxAbsDiff(scalarSim.getParticlePosZ(), simdSim.getParticlePosZ(), particleN));
        maxParticleTempDiff = std::max(
            maxParticleTempDiff, maxAbsDiff(scalarSim.getParticleTemperature(), simdSim.getParticleTemperature(), particleN));
        maxLavaDiff = std::max(maxLavaDiff, maxAbsDiff(scalarSim.getLavaHeightGrid(), simdSim.getLavaHeightGrid(), gridN));
        maxGridTempDiff = std::max(maxGridTempDiff, maxAbsDiff(scalarSim.getTemperatureGrid(), simdSim.getTemperatureGrid(), gridN));
        maxAvgLavaDiff = std::max(maxAvgLavaDiff, std::fabs(scalarSim.getAverageLavaHeight() - simdSim.getAverageLavaHeight()));
        maxAvgTempDiff = std::max(maxAvgTempDiff, std::fabs(scalarSim.getAverageTemperature() - simdSim.getAverageTemperature()));
    }

    std::cout << "Max abs diff over verification window:\n";
    std::cout << "  posX=" << maxPosXDiff << " posY=" << maxPosYDiff << " posZ=" << maxPosZDiff << '\n';
    std::cout << "  particleTemp=" << maxParticleTempDiff << " lavaGrid=" << maxLavaDiff << " tempGrid=" << maxGridTempDiff << '\n';
    std::cout << "  avgLava=" << maxAvgLavaDiff << " avgTemp=" << maxAvgTempDiff << '\n';

    auto startScalar = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < frames; ++i) {
        scalarSim.stepScalar(dt);
    }
    auto endScalar = std::chrono::high_resolution_clock::now();

    auto startSimd = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < frames; ++i) {
        simdSim.stepSIMD(dt);
    }
    auto endSimd = std::chrono::high_resolution_clock::now();

    const double scalarMs = std::chrono::duration<double, std::milli>(endScalar - startScalar).count();
    const double simdMs = std::chrono::duration<double, std::milli>(endSimd - startSimd).count();
    const double speedup = (simdMs > 0.0) ? (scalarMs / simdMs) : 0.0;

    std::cout << "Volcano simulation benchmark (" << frames << " frames)\n";
    std::cout << "Scalar: " << scalarMs << " ms\n";
    std::cout << "SIMD (AVX2): " << simdMs << " ms\n";
    std::cout << "Speedup: " << speedup << "x\n";
}
