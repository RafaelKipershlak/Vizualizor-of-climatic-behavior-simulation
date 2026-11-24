#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

constexpr float PI = 3.14159265358979323846f;

// =========================================================================
// Binary Contract — Matches Vertex.h byte-for-byte (std430 aligned)
// =========================================================================
struct FrameHeader {
    uint32_t magicNumber = 0x434C494D; // "CLIM"
    uint32_t version = 1;
    uint64_t timestampUnix = 0;        // Simulation timestamp (seconds)
    uint32_t particleCount = 0;        // Up to 5,000,000+
    uint32_t gridCellCount = 0;        // Spatial partition cells
    float    minTemp = 0.0f;           // Dataset metadata for UI normalization
    float    maxTemp = 0.0f;
    uint32_t reserved[4]{};            // Pads header to 48 bytes (multiple of 16)
};
static_assert(sizeof(FrameHeader) == 48, "FrameHeader must be 48 bytes to keep Particle[0] 16-byte aligned.");

struct alignas(16) Particle {
    float    position[3];       // Offset 0  (12 bytes): Cartesian (x, y, z)
    float    temperature;       // Offset 12 ( 4 bytes): Temperature
    float    velocity[3];       // Offset 16 (12 bytes): Wind vector (vx, vy, vz)
    uint32_t packedAttributes;  // Offset 28 ( 4 bytes): [Humidity(8b), CloudWater(8b), LatentHeat(8b), PhaseFlags(8b)]
};
static_assert(sizeof(Particle) == 32, "Particle must be strictly 32 bytes for std430 alignment.");

// Pack 4 normalized atmospheric attributes into 1 uint32_t (4 bytes)
static uint32_t packAttributes(float humidityNorm, float cloudWaterNorm,
                               float latentHeatNorm, uint8_t phaseFlags) {
    auto toByte = [](float v) -> uint32_t {
        return static_cast<uint32_t>(std::round(std::clamp(v, 0.0f, 1.0f) * 255.0f));
    };
    return (toByte(humidityNorm)   << 0)  |
           (toByte(cloudWaterNorm) << 8)  |
           (toByte(latentHeatNorm) << 16) |
           (static_cast<uint32_t>(phaseFlags) << 24);
}

// =========================================================================
// Advisor Coordinate System Helpers
// Matches simulation.cpp & trajectory-visualizer.cpp:
//   lat = asin(-z / r)   -> -z is North Pole (+90 deg)
//   lon = atan2(y, -x)   -> -x is Prime Meridian (0 deg)
// =========================================================================
struct Vec3 {
    float x, y, z;
};

static Vec3 sphericalToCartesian(float r, float lat, float lon) {
    float cosLat = std::cos(lat);
    return {
        -r * cosLat * std::cos(lon),
         r * cosLat * std::sin(lon),
        -r * std::sin(lat)
    };
}

static void cartesianToSpherical(const Vec3& p, float& r, float& lat, float& lon) {
    r = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
    if (r < 1e-6f) {
        lat = 0.0f;
        lon = 0.0f;
        return;
    }
    float sinLat = std::clamp(-p.z / r, -1.0f, 1.0f);
    lat = std::asin(sinLat);
    lon = std::atan2(p.y, -p.x);
}

// =========================================================================
// Synthetic 3-Cell Atmospheric Circulation Model
// Models Hadley (0-30°), Ferrel (30-60°), and Polar (60-90°) cells
// in the advisor's dimensionless units (R=30, H=40, T in [0.1,10], q in [0,0.1])
// =========================================================================
struct SyntheticParticle {
    float r, lat, lon;
    int   type;
};

struct ClimateSample {
    Vec3     pos;
    Vec3     vel;
    float    T;
    float    q;
    uint32_t packed;
};

static ClimateSample evaluateClimateField(const SyntheticParticle& sp, float R, float H, float time) {
    float hNorm = std::clamp((sp.r - R) / H, 0.0f, 1.0f);
    float absLat = std::abs(sp.lat);

    // 1. 3-Cell Overturning (Hadley: 0-30°, Ferrel: 30-60°, Polar: 60-90°)
    float cellPhase = 6.0f * sp.lat;
    float vMeridional = -1.2f * std::sin(cellPhase) * std::cos(PI * hNorm);
    float vVertical   =  0.6f * std::cos(cellPhase) * std::sin(PI * hNorm);

    // 2. Zonal Winds (Trade Winds, Mid-Latitude Westerlies + Jet Stream, Polar Easterlies)
    float vZonal = 0.0f;
    if (absLat < PI / 6.0f) {
        vZonal = -2.0f * std::cos(3.0f * absLat) * (1.0f - 1.5f * hNorm);
    } else if (absLat < PI / 3.0f) {
        float jet = std::sin(3.0f * (absLat - PI / 6.0f));
        vZonal = (2.2f + 2.8f * std::exp(-std::pow((hNorm - 0.65f) / 0.25f, 2.0f))) * jet;
    } else {
        vZonal = -1.5f * std::cos(3.0f * (absLat - PI / 3.0f));
    }

    // Rossby wave undulation
    float wave = 0.45f * std::sin(4.0f * sp.lon - 0.15f * time);
    vMeridional += wave * std::cos(sp.lat);

    // 3. Temperature T in advisor range [0.1, 10.0]
    float cosLat = std::cos(sp.lat);
    float T = 0.4f + 4.2f * (cosLat * cosLat) * std::exp(-1.4f * hNorm)
            + 0.25f * std::sin(sp.lon - 0.05f * time) * cosLat;
    T = std::clamp(T, 0.1f, 10.0f);

    // 4. Humidity q in advisor range [0.0, 0.1]
    float itcz = std::exp(-std::pow(sp.lat / 0.35f, 2.0f));
    float stormTrack = 0.6f * std::exp(-std::pow((absLat - 0.85f) / 0.25f, 2.0f));
    float q = 0.09f * std::clamp(itcz + stormTrack, 0.0f, 1.0f) * std::exp(-2.5f * hNorm);
    q = std::clamp(q, 0.0f, 0.1f);

    // Derive packed attributes: Humidity, CloudWater, LatentHeat, PhaseFlags
    float humidNorm  = q / 0.1f;
    float cloudNorm  = (humidNorm > 0.55f && hNorm > 0.15f && hNorm < 0.65f) ? (humidNorm - 0.55f) / 0.45f : 0.0f;
    float latentNorm = std::clamp(cloudNorm * std::max(0.0f, vVertical), 0.0f, 1.0f);
    uint8_t phaseFlags = 0;
    if (cloudNorm > 0.1f)  phaseFlags |= (1u << 0); // Bit 0: Condensation / Cloud
    if (cloudNorm > 0.65f) phaseFlags |= (1u << 1); // Bit 1: Precipitation

    // Convert spherical basis vectors (e_r, e_lat, e_lon) to Cartesian velocity
    float sinLat = std::sin(sp.lat);
    float cosLon = std::cos(sp.lon);
    float sinLon = std::sin(sp.lon);

    Vec3 e_r   = { -cosLat * cosLon,  cosLat * sinLon, -sinLat };
    Vec3 e_lat = {  sinLat * cosLon, -sinLat * sinLon, -cosLat };
    Vec3 e_lon = {  sinLon,           cosLon,           0.0f   };

    Vec3 vel = {
        vVertical * e_r.x + vMeridional * e_lat.x + vZonal * e_lon.x,
        vVertical * e_r.y + vMeridional * e_lat.y + vZonal * e_lon.y,
        vVertical * e_r.z + vMeridional * e_lat.z + vZonal * e_lon.z
    };

    return {
        sphericalToCartesian(sp.r, sp.lat, sp.lon),
        vel,
        T,
        q,
        packAttributes(humidNorm, cloudNorm, latentNorm, phaseFlags)
    };
}

static void initSyntheticParticles(std::vector<SyntheticParticle>& list, uint32_t count, float R, float H) {
    list.resize(count);
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> u01(0.0f, 1.0f);

    float rMin3 = R * R * R;
    float rMax3 = (R + H) * (R + H) * (R + H);

    for (uint32_t i = 0; i < count; ++i) {
        float r   = std::cbrt(rMin3 + u01(rng) * (rMax3 - rMin3));
        float lat = std::asin(2.0f * u01(rng) - 1.0f);
        float lon = (2.0f * u01(rng) - 1.0f) * PI;
        list[i] = { r, lat, lon, static_cast<int>(i % 2) };
    }
}

static void advectParticles(std::vector<SyntheticParticle>& list, const std::vector<ClimateSample>& samples,
                            float R, float H, float dt) {
    for (size_t i = 0; i < list.size(); ++i) {
        Vec3 nextPos = {
            samples[i].pos.x + samples[i].vel.x * dt,
            samples[i].pos.y + samples[i].vel.y * dt,
            samples[i].pos.z + samples[i].vel.z * dt
        };
        float r, lat, lon;
        cartesianToSpherical(nextPos, r, lat, lon);
        list[i].r   = std::clamp(r, R + 0.25f, R + H - 0.25f);
        list[i].lat = std::clamp(lat, -PI * 0.495f, PI * 0.495f);
        list[i].lon = lon;
    }
}

static std::string formatFrameName(const std::string& ext, uint32_t frameIdx) {
    std::ostringstream oss;
    oss << "frame_" << std::setw(6) << std::setfill('0') << frameIdx << "." << ext;
    return oss.str();
}

static bool writeBinaryFrame(const fs::path& filepath, double timestamp, const std::vector<Particle>& particles) {
    std::ofstream ofs(filepath, std::ios::binary);
    if (!ofs) {
        std::cerr << "Error: Cannot write " << filepath << "\n";
        return false;
    }

    float minT = std::numeric_limits<float>::max();
    float maxT = std::numeric_limits<float>::lowest();
    for (const auto& p : particles) {
        minT = std::min(minT, p.temperature);
        maxT = std::max(maxT, p.temperature);
    }
    if (particles.empty()) {
        minT = 0.0f;
        maxT = 0.0f;
    }

    FrameHeader header{};
    header.timestampUnix = static_cast<uint64_t>(std::max(0.0, timestamp));
    header.particleCount = static_cast<uint32_t>(particles.size());
    header.gridCellCount = 0;
    header.minTemp = minT;
    header.maxTemp = maxT;

    ofs.write(reinterpret_cast<const char*>(&header), sizeof(FrameHeader));
    ofs.write(reinterpret_cast<const char*>(particles.data()), particles.size() * sizeof(Particle));
    return static_cast<bool>(ofs);
}

// =========================================================================
// Mode 1: Direct Synthetic .bin Generator (Fast for 1M / 5M / 10M datasets)
// =========================================================================
static int runGenerateBin(const fs::path& outDir, uint32_t numParticles, uint32_t numFrames) {
    fs::create_directories(outDir);
    const float R = 30.0f, H = 40.0f, dt = 1.0f;

    std::vector<SyntheticParticle> state;
    initSyntheticParticles(state, numParticles, R, H);

    std::vector<ClimateSample> samples(numParticles);
    std::vector<Particle> gpuParticles(numParticles);

    for (uint32_t f = 0; f < numFrames; ++f) {
        float time = f * dt;
        for (uint32_t i = 0; i < numParticles; ++i) {
            samples[i] = evaluateClimateField(state[i], R, H, time);
            gpuParticles[i].position[0]      = samples[i].pos.x;
            gpuParticles[i].position[1]      = samples[i].pos.y;
            gpuParticles[i].position[2]      = samples[i].pos.z;
            gpuParticles[i].temperature      = samples[i].T;
            gpuParticles[i].velocity[0]      = samples[i].vel.x;
            gpuParticles[i].velocity[1]      = samples[i].vel.y;
            gpuParticles[i].velocity[2]      = samples[i].vel.z;
            gpuParticles[i].packedAttributes = samples[i].packed;
        }

        fs::path outFile = outDir / formatFrameName("bin", f);
        if (!writeBinaryFrame(outFile, static_cast<double>(time), gpuParticles)) return 1;
        std::cout << "[generate-bin] Wrote " << outFile.filename().string()
                  << " (" << fs::file_size(outFile) << " bytes)\n";

        advectParticles(state, samples, R, H, dt);
    }
    return 0;
}

// =========================================================================
// Mode 2: Synthetic JSON Generator (Matches Advisor io.cpp Schema)
// =========================================================================
static int runGenerateJson(const fs::path& outDir, uint32_t numParticles, uint32_t numFrames) {
    fs::create_directories(outDir);
    const float R = 30.0f, H = 40.0f, dt = 1.0f;

    std::vector<SyntheticParticle> state;
    initSyntheticParticles(state, numParticles, R, H);
    std::vector<ClimateSample> samples(numParticles);

    for (uint32_t f = 0; f < numFrames; ++f) {
        float time = f * dt;
        for (uint32_t i = 0; i < numParticles; ++i) {
            samples[i] = evaluateClimateField(state[i], R, H, time);
        }

        fs::path outFile = outDir / formatFrameName("json", f);
        std::ofstream ofs(outFile);
        if (!ofs) {
            std::cerr << "Error: Cannot write " << outFile << "\n";
            return 1;
        }

        ofs << std::fixed << std::setprecision(6);
        ofs << "{\n";
        ofs << "  \"frame\": " << f << ",\n";
        ofs << "  \"time\": " << time << ",\n";
        ofs << "  \"rotation_angle\": " << (0.006283185f * time) << ",\n";
        ofs << "  \"R\": " << R << ",\n";
        ofs << "  \"H\": " << H << ",\n";
        ofs << "  \"N\": " << numParticles << ",\n";
        ofs << "  \"types\": [\n"
            << "    {\"name\": \"Argon\", \"symbol\": \"Ar\"},\n"
            << "    {\"name\": \"Neon\", \"symbol\": \"Ne\"}\n"
            << "  ],\n";
        ofs << "  \"particles\": [\n";
        for (uint32_t i = 0; i < numParticles; ++i) {
            ofs << "    {\"x\": " << samples[i].pos.x
                << ", \"y\": " << samples[i].pos.y
                << ", \"z\": " << samples[i].pos.z
                << ", \"type\": " << state[i].type
                << ", \"T\": " << samples[i].T
                << ", \"q\": " << samples[i].q << "}";
            if (i + 1 < numParticles) ofs << ",";
            ofs << "\n";
        }
        ofs << "  ]\n}\n";
        ofs.close();

        std::cout << "[generate-json] Wrote " << outFile.filename().string() << "\n";
        advectParticles(state, samples, R, H, dt);
    }
    return 0;
}

// =========================================================================
// Mode 3: JSON-to-.bin Preprocessor (Works on Advisor & Synthetic JSONs)
// Derives velocity via finite differences between consecutive frames
// =========================================================================
struct RawJsonFrame {
    double time = 0.0;
    std::vector<Particle> particles;
};

static bool loadJsonFrame(const fs::path& filepath, RawJsonFrame& out) {
    std::ifstream ifs(filepath);
    if (!ifs) return false;

    json doc;
    ifs >> doc;

    out.time = doc.value("time", 0.0);
    const auto& arr = doc.at("particles");
    out.particles.resize(arr.size());

    for (size_t i = 0; i < arr.size(); ++i) {
        const auto& p = arr[i];
        float q = p.value("q", 0.0f);
        float humidNorm = std::clamp(q / 0.1f, 0.0f, 1.0f);

        out.particles[i].position[0]      = p.at("x").get<float>();
        out.particles[i].position[1]      = p.at("y").get<float>();
        out.particles[i].position[2]      = p.at("z").get<float>();
        out.particles[i].temperature      = p.at("T").get<float>();
        out.particles[i].velocity[0]      = 0.0f;
        out.particles[i].velocity[1]      = 0.0f;
        out.particles[i].velocity[2]      = 0.0f;
        out.particles[i].packedAttributes = packAttributes(humidNorm, 0.0f, 0.0f, 0);
    }
    return true;
}

static int runConvertJsonToBin(const fs::path& inDir, const fs::path& outDir) {
    fs::create_directories(outDir);

    std::vector<fs::path> jsonFiles;
    for (const auto& entry : fs::directory_iterator(inDir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string name = entry.path().filename().string();
            if (name.rfind("frame_", 0) == 0) {
                jsonFiles.push_back(entry.path());
            }
        }
    }
    std::sort(jsonFiles.begin(), jsonFiles.end());

    if (jsonFiles.empty()) {
        std::cerr << "Error: No frame_*.json files found in " << inDir << "\n";
        return 1;
    }

    RawJsonFrame prev, curr;
    if (!loadJsonFrame(jsonFiles[0], curr)) {
        std::cerr << "Error: Failed to parse " << jsonFiles[0] << "\n";
        return 1;
    }

    for (size_t f = 0; f < jsonFiles.size(); ++f) {
        RawJsonFrame next;
        bool hasNext = (f + 1 < jsonFiles.size());
        if (hasNext && !loadJsonFrame(jsonFiles[f + 1], next)) {
            std::cerr << "Error: Failed to parse " << jsonFiles[f + 1] << "\n";
            return 1;
        }

        if (hasNext && next.particles.size() == curr.particles.size()) {
            float invDt = (next.time > curr.time) ? static_cast<float>(1.0 / (next.time - curr.time)) : 1.0f;
            for (size_t i = 0; i < curr.particles.size(); ++i) {
                curr.particles[i].velocity[0] = (next.particles[i].position[0] - curr.particles[i].position[0]) * invDt;
                curr.particles[i].velocity[1] = (next.particles[i].position[1] - curr.particles[i].position[1]) * invDt;
                curr.particles[i].velocity[2] = (next.particles[i].position[2] - curr.particles[i].position[2]) * invDt;
            }
        } else if (f > 0 && prev.particles.size() == curr.particles.size()) {
            float invDt = (curr.time > prev.time) ? static_cast<float>(1.0 / (curr.time - prev.time)) : 1.0f;
            for (size_t i = 0; i < curr.particles.size(); ++i) {
                curr.particles[i].velocity[0] = (curr.particles[i].position[0] - prev.particles[i].position[0]) * invDt;
                curr.particles[i].velocity[1] = (curr.particles[i].position[1] - prev.particles[i].position[1]) * invDt;
                curr.particles[i].velocity[2] = (curr.particles[i].position[2] - prev.particles[i].position[2]) * invDt;
            }
        }

        fs::path outFile = outDir / (jsonFiles[f].stem().string() + ".bin");
        if (!writeBinaryFrame(outFile, curr.time, curr.particles)) return 1;
        std::cout << "[convert] " << jsonFiles[f].filename().string()
                  << " -> " << outFile.filename().string()
                  << " (" << curr.particles.size() << " particles, "
                  << fs::file_size(outFile) << " bytes)\n";

        prev = std::move(curr);
        curr = std::move(next);
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cout << "Usage:\n"
                  << "  ClimatePreprocessor generate-json <out_dir> <num_particles> <num_frames>\n"
                  << "  ClimatePreprocessor convert       <in_json_dir> <out_bin_dir>\n"
                  << "  ClimatePreprocessor generate-bin  <out_dir> <num_particles> <num_frames>\n";
        return 1;
    }

    std::string mode = argv[1];
    if (mode == "generate-bin" && argc == 5) {
        return runGenerateBin(argv[2], static_cast<uint32_t>(std::stoul(argv[3])), static_cast<uint32_t>(std::stoul(argv[4])));
    }
    if (mode == "generate-json" && argc == 5) {
        return runGenerateJson(argv[2], static_cast<uint32_t>(std::stoul(argv[3])), static_cast<uint32_t>(std::stoul(argv[4])));
    }
    if (mode == "convert" && argc == 4) {
        return runConvertJsonToBin(argv[2], argv[3]);
    }

    std::cerr << "Invalid arguments.\n";
    return 1;
}