#pragma once 
#include "IMesh.h"
#include "GLEW/glew.h"

class GLMesh : public IMesh {
    public:
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