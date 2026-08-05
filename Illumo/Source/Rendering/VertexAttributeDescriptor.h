#pragma once 

enum class VertexAttributeType {
    Float,
    Float2,
    Float3,
    Float4,
    Byte,
    UnsignedByte,
    Short,
    UnsignedShort,
    Int2_10_10_10_Rev // For those optimized packed normals!
};


struct VertexAttribute {
    uint32_t location;
    uint32_t componentCount;
    VertexAttributeType dataType; // e.g., GL_FLOAT, GL_UNSIGNED_BYTE
    uint32_t offset;
};

class VertexLayoutDescriptor {
public:
    uint32_t stride = 0;
    std::vector<VertexAttribute> attributes;

    void AddAttribute(uint32_t location, uint32_t count, VertexAttributeType type, uint32_t sizeOfComponent) {
        attributes.push_back({location, count, type, stride});
        stride += (count * sizeOfComponent); // Automatically calculates stride dynamically
    }
};