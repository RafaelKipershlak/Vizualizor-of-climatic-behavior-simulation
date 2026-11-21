#pragma once

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

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

    float padding[2];      // Offsets 8 and 12

    // C++ automatically adds 8 bytes of padding here for 16-byte alignment of mat4
    float projectionData[4][4]; // Fixed-size raw array of 16 floats
    float viweData[4][4]; // Fixed-size raw array of 16 floats

};



// =========================================================================
// 1. EarthVertex (3D Globe Mesh)
// =========================================================================
struct EarthVertex
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;

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
// 2. Particle (Strictly 32-byte std430 layout for .bin mmap & GPU SSBOs)
// =========================================================================
struct alignas(16) Particle
{
    glm::vec4 posTemp;  // xyz = 3D position, w = temperature (°C)
    glm::vec4 velHumid; // xyz = 3D velocity, w = humidity / water vapor

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Particle);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 2> attrs{};

        attrs[0].binding = 0;
        attrs[0].location = 0;
        attrs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrs[0].offset = offsetof(Particle, posTemp);

        attrs[1].binding = 0;
        attrs[1].location = 1;
        attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrs[1].offset = offsetof(Particle, velHumid);

        return attrs;
    }
};
static_assert(sizeof(Particle) == 32, "Particle must be exactly 32 bytes for std430 alignment.");

// =========================================================================
// 3. FrameHeader (32-byte binary header at the start of each .bin frame)
// =========================================================================
struct FrameHeader
{
    uint32_t magic = 0x434C494D; // "CLIM" file signature
    uint32_t version = 1;
    uint32_t particleCount = 0;
    uint32_t flags = 0;
    double timestamp = 0.0;      // Simulation timestamp (t_N)
    uint64_t reserved = 0;       // Pads header to 32 bytes so Particle[0] is 16-byte aligned
};
static_assert(sizeof(FrameHeader) == 32, "FrameHeader must be 32 bytes to keep Particle data 16-byte aligned.");

// =========================================================================
// 4. FrameUniforms (Per-frame UBO shared by Compute & Graphics shaders)
// =========================================================================
struct alignas(16) FrameUniforms
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;

    glm::vec4 cameraPos;         // xyz = camera world position, w = camera altitude
    glm::vec4 frustumPlanes[6];  // Left, Right, Bottom, Top, Near, Far (xyz = normal, w = d)

    glm::vec4 simParams;         // x = alpha [0..1], y = particleSize, z = opacity, w = simTime
    glm::vec4 lodAltitudeParams; // x = lodDist1 (d1), y = lodDist2 (d2), z = minAlt, w = maxAlt
    glm::uvec4 layerParams;      // x = activeLayers bitmask (9 layers), y = colorMapMode, z = totalParticles, w = reserved
};
static_assert(sizeof(FrameUniforms) % 16 == 0, "FrameUniforms must be a multiple of 16 bytes for std140.");

// =========================================================================
// 5. ComputeOutputMeta (Indirect Draw Command + Live Viewport Histograms)
// =========================================================================
struct alignas(16) ComputeOutputMeta
{
    // First 16 bytes match VkDrawIndirectCommand so vkCmdDrawIndirect can read directly from offset 0
    uint32_t vertexCount = 0;    // Atomically incremented by Compute Shader for visible particles
    uint32_t instanceCount = 1;
    uint32_t firstVertex = 0;
    uint32_t firstInstance = 0;

    // Real-time viewport analytics bins (read back by UI panel)
    static constexpr uint32_t HISTOGRAM_BINS = 16;
    uint32_t tempHistogram[HISTOGRAM_BINS]{};
    uint32_t windHistogram[HISTOGRAM_BINS]{};
    uint32_t humidityHistogram[HISTOGRAM_BINS]{};
};
static_assert(sizeof(ComputeOutputMeta) == 16 + (16 * 3 * 4), "ComputeOutputMeta layout mismatch.");