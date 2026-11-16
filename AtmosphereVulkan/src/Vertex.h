#pragma once

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