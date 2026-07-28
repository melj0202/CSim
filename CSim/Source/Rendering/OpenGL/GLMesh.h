#pragma once 
#include "IMesh.h"
#include <GL/glew.h>

class GLMesh : public IMesh {
    public:
    GLMesh(const void* vertices, size_t vertexSize, const void* indices, size_t indexSize) {
        if (vertices && vertexSize > 0) {
            const float* floatVerts = static_cast<const float*>(vertices);
            size_t floatCount = vertexSize / sizeof(float);
            _vertexData.assign(floatVerts, floatVerts + floatCount);
        }
        if (indices && indexSize > 0) {
            const unsigned int* uintIndices = static_cast<const unsigned int*>(indices);
            size_t indexCount = indexSize / sizeof(unsigned int);
            _indexData.assign(uintIndices, uintIndices + indexCount);
        }
        glGenVertexArrays(1, &_vaoID);
        glGenBuffers(1, &_vboID);
        glGenBuffers(1, &_eboID);
    }

    GLMesh(const std::vector<float>& vertexData, const std::vector<unsigned int>& indexData) {
        glGenVertexArrays(1, &_vaoID);
        glGenBuffers(1, &_vboID);
        glGenBuffers(1, &_eboID);
    }

    GLMesh(const std::vector<float>& vertexData) {
        glGenVertexArrays(1, &_vaoID);
        glGenBuffers(1, &_vboID);
    }
    ~GLMesh() = default;

    void Destroy() override {
        glDeleteVertexArrays(1, &_vaoID);
        glDeleteBuffers(1, &_vboID);
        glDeleteBuffers(1, &_eboID);
    }

    
};