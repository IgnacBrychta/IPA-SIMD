#ifndef VOLCANO_SIM_H
#define VOLCANO_SIM_H

#include <cstddef>
#include <vector>

class Terrain;

class VolcanoSimulation {
public:
    struct RenderConfig {
        float worldX;
        float worldY;
        float worldZ;
        float coneRadius;
        float coneHeight;
        float lavaFieldRadius;
        float particleSpreadScale;
    };

    VolcanoSimulation();
    ~VolcanoSimulation();

    bool init(std::size_t particleCount, int gridWidth, int gridHeight, unsigned int seed = 1337u);

    void stepScalar(float dt);
    void stepSIMD(float dt);

    std::size_t getParticleCount() const { return particles.size(); }
    const float* getParticlePosX() const { return particles.posX.data(); }
    const float* getParticlePosY() const { return particles.posY.data(); }
    const float* getParticlePosZ() const { return particles.posZ.data(); }
    const float* getParticleTemperature() const { return particles.temperature.data(); }

    int getGridWidth() const { return width; }
    int getGridHeight() const { return height; }
    const float* getLavaHeightGrid() const { return lavaHeight.data(); }
    const float* getTemperatureGrid() const { return temperature.data(); }

    void setRenderConfig(const RenderConfig& config) { renderConfig = config; }
    const RenderConfig& getRenderConfig() const { return renderConfig; }
    void setCollisionTerrain(const Terrain& terrain, float waterLevelWorld = 0.0f);

    float getAverageLavaHeight() const { return averageLavaHeight; }
    float getAverageTemperature() const { return averageTemperature; }

private:
    class AlignedFloatArray {
    public:
        AlignedFloatArray();
        ~AlignedFloatArray();

        AlignedFloatArray(const AlignedFloatArray& other);
        AlignedFloatArray& operator=(const AlignedFloatArray& other);
        AlignedFloatArray(AlignedFloatArray&& other) noexcept;
        AlignedFloatArray& operator=(AlignedFloatArray&& other) noexcept;

        bool resize(std::size_t count);
        float* data();
        const float* data() const;
        std::size_t size() const;

    private:
        float* ptr;
        std::size_t count;
    };

    struct ParticleSoA {
        AlignedFloatArray posX;
        AlignedFloatArray posY;
        AlignedFloatArray posZ;
        AlignedFloatArray velX;
        AlignedFloatArray velY;
        AlignedFloatArray velZ;
        AlignedFloatArray life;
        AlignedFloatArray temperature;

        bool resize(std::size_t count);
        std::size_t size() const;
    };

    ParticleSoA particles;

    int width;
    int height;

    AlignedFloatArray terrainHeight;
    AlignedFloatArray lavaHeight;
    AlignedFloatArray lavaHeightNext;
    AlignedFloatArray temperature;
    AlignedFloatArray temperatureNext;

    float gravity;
    float damping;
    float particleCooling;
    float lavaViscosity;
    float lavaCooling;
    float diffusionAlpha;
    float ambientTemperature;
    float ventTemperature;
    float lavaSourceRate;
    float windX;
    float windZ;
    float plumeBuoyancy;
    float eruptionClock;
    float ventPulseLevel;
    RenderConfig renderConfig;
    float averageLavaHeight;
    float averageTemperature;

    int collisionTerrainSize;
    float collisionTerrainOriginX;
    float collisionTerrainOriginZ;
    float collisionTerrainSpacing;
    float collisionWaterLevelWorld;
    std::vector<float> collisionTerrainHeights;

    unsigned int rngState;

    std::size_t idx(int x, int y) const;
    float random01();
    float sampleCollisionTerrainHeightWorld(float worldX, float worldZ) const;
    void sampleCollisionTerrainGradientWorld(float worldX, float worldZ, float& dHdX, float& dHdZ) const;
    void respawnParticle(std::size_t i);

    void initTerrainAndLava();
    void injectVentSources(float dt);

    void updateParticlesScalar(float dt);
    void updateParticlesSIMD(float dt);

    void updateLavaFluxScalar(float dt);
    void updateLavaFluxSIMD(float dt);

    void diffuseHeatScalar(float dt);
    void diffuseHeatSIMD(float dt);

    void updateStats();
};

void runVolcanoBenchmark();
void runVolcanoReference(int frames = 10);

#endif
