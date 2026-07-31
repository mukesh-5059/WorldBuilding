# Hardcoded Parameters & Default Configurations (`IslandTesting`)

This document records all **hardcoded mathematical constants**, **noise parameters**, **shape refinement values**, and **default starting configurations** used in the 2D Island Coastline Generator (`src/apps/IslandTesting`).

---

## 1. Hardcoded FastNoise Lite Constants

These parameters are compiled as `constexpr` constants in [`IslandTesting.hpp`](file:///home/mukes/dev/C++/Projects/WorldBuilding/src/apps/IslandTesting/IslandTesting.hpp#L8-L12) and applied directly during heightmap generation:

| Parameter | C++ Variable Name | Hardcoded Value | Description |
| :--- | :--- | :--- | :--- |
| **Noise Type** | `HARDCODED_NOISE_TYPE` | `FastNoiseLite::NoiseType_OpenSimplex2` | Smooth OpenSimplex2 basis noise. |
| **Fractal Type** | `HARDCODED_FRACTAL_TYPE` | `FastNoiseLite::FractalType_Ridged` | Sharp ridged fractal octave blending. |
| **Octaves** | `HARDCODED_OCTAVES` | `4` | Number of fractal detail layers. |
| **Gain / Persistence** | `NOISE_GAIN` | `0.4f` | Amplitude decay factor per octave layer. |
| **Lacunarity** | `NOISE_LACUNARITY` | `2.1f` | Frequency multiplier per octave layer. |

---

## 2. Hardcoded Shape Refinement Constants

These geometric constants in [`IslandTesting.hpp`](file:///home/mukes/dev/C++/Projects/WorldBuilding/src/apps/IslandTesting/IslandTesting.hpp#L14-L18) refine Starfish and Diamond falloff shapes to prevent unwanted artifacts:

| Parameter | C++ Variable Name | Hardcoded Value | Description |
| :--- | :--- | :--- | :--- |
| **Starfish Max Arms** | `HARDCODED_STARFISH_ARMS` | `10.0f` | Upper bound for per-seed arm count randomization. |
| **Starfish Base Depth** | `HARDCODED_STARFISH_AMP` | `0.15f` | Base wave modulation depth ($A$) between peninsula arms. |
| **Starfish Spike Dampening** | `HARDCODED_STARFISH_DAMPENING` | `2.24f` | Exponent dampening arm spikes on small outer seed centers. |
| **Diamond Pinch ($q$)** | `HARDCODED_DIAMOND_PINCH` | `1.19f` | Superellipse norm exponent $q$ ($1.19 \implies$ smooth rounded diamond). |

---

## 3. Automated Per-Seed Randomization Bounds

When generating multi-seed islands, the following properties are randomized deterministically per seed center $P_i$ using hash noise:

| Property | Randomized Range / Formula | Purpose |
| :--- | :--- | :--- |
| **Starfish Arm Count** | $\text{int} \in [3, \; 10]$ | Gives each Starfish seed a different number of radiating arms. |
| **Starfish Amplitude Variance** | $\text{clamp}(0.15 \times (0.8 + \text{rand} \times 0.4), \; 0.05, \; 0.60)$ | Natural variation in peninsula bay depth. |
| **Diamond Rotation Angle** | $\theta \in [0.0^\circ, \; 90.0^\circ]$ | Rotates diamond headlands in random orientations. |
| **Diamond Aspect Ratio** | $k_{\text{aspect}} \in [0.60, \; 1.60]$ | Elongates diamond seeds into varied capes and peninsulas. |

---

## 4. Default Application Configuration (`IslandConfig`)

Starting UI inspector values loaded upon launch:

```cpp
struct IslandConfig {
    int mapWidth = 256;               // Grid Resolution Width
    int mapHeight = 256;              // Grid Resolution Height
    int seed = 328;                   // Default Map Seed

    float waterLevel = 0.146f;        // Sea level slicing plane
    float coastLineWidth = 0.015f;    // Golden coastline contour outline width

    FalloffMode falloffMode = FalloffMode::MultiSeedMetaball; // Organic potential blending
    FalloffType falloffType = FalloffType::RandomPerSeed;     // Per-seed shape randomization

    int seedCount = 12;               // Number of island seed centers
    float seedSpread = 0.84f;         // Spread radius of seed centers across the map
    float seedMinRadius = 0.28f;      // Min influence radius per seed
    float seedMaxRadius = 0.70f;      // Max influence radius per seed
    float falloffPower = 1.41f;       // Falloff curve exponent

    float frequency = 1.50f;          // Scale-invariant noise frequency
    float details = 10.88f;           // Details multiplier
};
```

---

## 5. Grid-Resolution Invariance Formula

Noise sampling coordinates are normalized to UV space $[0, 1]$ before scaling, ensuring island coastline shapes remain **100% identical regardless of resolution** (128x128, 256x256, 512x512, 1024x1024):

$$u = \frac{x}{\text{mapWidth}}, \qquad v = \frac{y}{\text{mapHeight}}$$

$$\text{noiseScale} = \text{frequency} \times \text{details} \times 10.0$$

$$\text{rawNoise} = \text{FastNoise Lite}.\text{GetNoise}(u \times \text{noiseScale}, \; v \times \text{noiseScale})$$
