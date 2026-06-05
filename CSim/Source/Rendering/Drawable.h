#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include "Init/MacroDefs.h"

// 1. Thin non-templated base class for storing in RenderQueue
class DrawableBase {
public:
    virtual ~DrawableBase() {
        if (shaderID) {
            glDeleteShader(*shaderID);
            delete shaderID;
        }
        if (shaderProgramID) {
            glDeleteProgram(*shaderProgramID);
            delete shaderProgramID;
        }
        if (VAO) {
            glDeleteVertexArrays(1, VAO);
            delete VAO;
        }
        if (VBO) {
            glDeleteBuffers(1, VBO);
            delete VBO;
        }
        if (EBO) {
            glDeleteBuffers(1, EBO);
            delete EBO;
        }
    }
    virtual void Draw() = 0; // Virtual dispatch at the very top level

    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

protected:
    bool visible = true;
    unsigned int* shaderID = 0;
    unsigned int* shaderProgramID = 0;
    unsigned int* VAO = 0;
    unsigned int* VBO = 0;
    unsigned int* EBO = 0;

    unsigned int compileShader(unsigned int type, const char* shaderSource);
    unsigned int createShaderProgram(const char* vertexSource, const char* fragmentSource);
};

// 2. CRTP Template class
template <typename Derived>
class Drawable : public DrawableBase {
public:
    // Implements the virtual Draw from the base class
    __CSIM_FORCE_INLINE__ void Draw() override {
        if (isVisible()) {
            // Compile-time static dispatch to the Derived class implementation
           static_cast<Derived*>(this)->DrawImpl();
        }
    }
};
