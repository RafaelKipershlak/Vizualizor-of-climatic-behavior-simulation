#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan.h>
#include "Camera.h"

struct Vertex
{
    float x;
    float y;

    float r;
    float g;
    float b;
};

struct VertexCube
{
    float x;
    float y;
    float z;
};

struct PushConstants
{
    float angleX;
    float angleY;

    std::array<float, 2> padding; // Offsets 8 and 12

    // C++ automatically adds 8 bytes of padding here for 16-byte alignment of mat4
    Camera::Mat4 projection;
    Camera::Mat4 view;

};


// =========================================================================
// 1. EarthVertex (3D Globe Mesh)
// =========================================================================
struct EarthVertex
{
    float pos[3];
    float normal[3];
    float uv[2];

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(EarthVertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 3> attrs{};

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(EarthVertex, pos);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset = offsetof(EarthVertex, normal);

        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[2].offset = offsetof(EarthVertex, uv);

        return attrs;
    }
};

// =========================================================================
// 2. FrameHeader (48-byte binary header at the start of each .bin frame)
// =========================================================================
struct FrameHeader
{
    uint32_t magicNumber = 0x434C494D; // "CLIM" file signature
    uint32_t version = 1;
    uint64_t timestampUnix = 0;        // Simulation timestamp (seconds)
    uint32_t particleCount = 0;        // Up to 5,000,000
    uint32_t gridCellCount = 0;        // Number of spatial partition cells
    float    minTemp = 0.0f;           // Dataset metadata for UI normalization
    float    maxTemp = 0.0f;
    uint32_t reserved[4]{};            // Pads header to 48 bytes (multiple of 16)
};
static_assert(sizeof(FrameHeader) == 48, "FrameHeader must be 48 bytes to keep Particle[0] 16-byte aligned.");

// =========================================================================
// 3. Particle (Strictly 32-byte std430 layout for .bin mmap & GPU SSBOs)
// =========================================================================
struct alignas(16) Particle
{
    float    position[3];       // 12 bytes: Cartesian (x, y, z) on/above Earth sphere (Offset 0)
    float    temperature;       //  4 bytes: Kelvin / Celsius                          (Offset 12)
    float    velocity[3];       // 12 bytes: Wind vector (vx, vy, vz) m/s              (Offset 16)
    uint32_t packedAttributes;  //  4 bytes: [Humidity(8b), CloudWater(8b), LatentHeat(8b), PhaseFlags(8b)] (Offset 28)

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Particle);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 4> attrs{};

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = offsetof(Particle, position);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32_SFLOAT;
        attrs[1].offset = offsetof(Particle, temperature);

        attrs[2].binding = 0;
        attrs[2].location = 2;
        attrs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[2].offset = offsetof(Particle, velocity);

        attrs[3].binding = 0;
        attrs[3].location = 3;
        attrs[3].format = VK_FORMAT_R32_UINT;
        attrs[3].offset = offsetof(Particle, packedAttributes);

        return attrs;
    }
};
static_assert(sizeof(Particle) == 32, "Particle must be strictly 32 bytes for std430 alignment.");

// =========================================================================
// 4. ComputeOutputMeta (Indirect Draw Command + Live Viewport Histograms)
// =========================================================================
struct alignas(16) ComputeOutputMeta
{
    // First 16 bytes match VkDrawIndirectCommand at offset 0
    uint32_t vertexCount = 0;   // Atomically incremented by Compute Shader for visible particles
    uint32_t instanceCount = 1;
    uint32_t firstVertex = 0;
    uint32_t firstInstance = 0;

    // Real-time viewport analytics bins (32 bins each) for the Info & Analytics Panel
    uint32_t tempHistogram[32]{};
    uint32_t windSpeedHistogram[32]{};
    uint32_t humidityHistogram[32]{};
    uint32_t altitudeTempSum[32]{};
    uint32_t altitudeParticleCount[32]{};
};
static_assert(sizeof(ComputeOutputMeta) == 16 + (5 * 32 * 4), "ComputeOutputMeta layout mismatch.");

// =========================================================================
// 5. FrameUniforms (Per-frame UBO shared by Compute & Graphics shaders)
// =========================================================================
struct alignas(16) FrameUniforms
{
    float    viewProj[16];          // Column-major 4x4 View-Projection matrix (64 bytes)
    float    cameraPosAndFOV[4];    // xyz = Camera Pos, w = FOV               (16 bytes)
    float    frustumPlanes[6][4];   // 6 frustum planes for Compute Culling    (96 bytes)

    float    alpha = 0.0f;          // Temporal interpolation factor [0.0, 1.0]
    float    particleSize = 2.5f;   // From Left UI Panel
    float    opacity = 0.75f;       // From Left UI Panel
    float    minAltitudeKm = 0.0f;  // From Bottom-Right UI Panel

    float    maxAltitudeKm = 80.0f; // From Bottom-Right UI Panel
    uint32_t activeLayerMask = 0;   // 9-bit bitmask for the 9 Atmospheric Phenomena checkboxes
    uint32_t colorMapMode = 0;      // 0 = Temp, 1 = WindSpeed, 2 = Humidity, 3 = LatentHeat
    float    lodDist1 = 0.0f;       // d1 threshold for LOD1

    float    lodDist2 = 0.0f;       // d2 threshold for LOD2
    float    _pad[3]{};             // Explicit padding to 224 bytes (multiple of 16 for std140)
};
static_assert(sizeof(FrameUniforms) % 16 == 0, "FrameUniforms must be a multiple of 16 bytes for std140.");