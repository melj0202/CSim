#pragma once
#include "IBackend.h"
#include "OpenGL/GLBackend.h"
#include "IRenderWindow.h"
#include "EnvVars.h"
#include "IShaderProgram.h"
#include "OpenGL/GLShaderProgram.h"
#include "Camera.h"
#include <vector>
#include "Scene.h"
#include "RenderPass.h"

struct FrameBuffer {
    unsigned int id = 0;
    unsigned int width = 0;
    unsigned int height = 0;
};

struct Uniform {
    std::string name;
    unsigned int location = 0;
};

class Renderer {
private:
    IBackend* _backend;
    IRenderWindow* _window;
    Camera* _camera;
    EnvVars* envVars;
    Scene* currentScene;
    std::vector<PipelineState> pipelineStates;
    std::vector<RenderPass> renderPasses;
    std::vector<FrameBuffer> frameBuffers;
    std::vector<Uniform> uniforms;

public:
    Renderer(IRenderWindow* window, EnvVars* envVars, Camera* cam) : _window(window), envVars(envVars), _camera(cam), _backend(nullptr) {
        std::string api = envVars->getVar("GraphicsAPI").value;
        
        if (api == "OpenGL") {
            _backend = new GLBackend(_window);
        } else {
            // Future placeholder (e.g., Vulkan/DX12), defaulting safely for now
            _backend = new GLBackend(_window);
        }
    }

    // Fixed: Standard rule-of-three memory cleanup for the backend raw pointer
    ~Renderer() {
        _backend->Shutdown();
        delete _backend;
    }

    // =========================================================================
    // 1. ASSET ENROLLMENT (Direct creation, no command queue pollution)
    // =========================================================================

    unsigned long enrollShader(const ShaderPaths& paths, unsigned long tableID) {
        return _backend->CreateShaderProgram(paths, tableID);
    }

    unsigned long enrollShader(const ShaderSources& sources, unsigned long tableID) {
        return _backend->CreateShaderProgram(sources, tableID);
    }

    unsigned long enrollMesh(const void* vertices, const size_t verticesSize, const void* indices, const size_t indicesSize, unsigned long tableID) {
        return _backend->CreateMesh(vertices, verticesSize, indices, indicesSize, tableID);
    }

    unsigned long enrollMesh(std::string filePath, unsigned long tableID) {
        return _backend->CreateMesh(filePath, tableID);
    }

    unsigned long enrollTexture(const std::string& filePath, unsigned long tableID) {
        return _backend->CreateTexture(filePath, tableID);
    }

    unsigned long enrollTexture(const unsigned char* data, const int width, const int height, unsigned long tableID) {
        return _backend->CreateTexture(data, width, height, tableID);
    }

    unsigned long enrollRenderPass(const RenderPass& renderPass) {
        renderPasses.push_back(renderPass);
        return renderPasses.size() - 1;
    }

    unsigned long enrollFrameBuffer(const FrameBuffer& frameBuffer) {
        frameBuffers.push_back(frameBuffer);
        return frameBuffers.size() - 1;
    }

    unsigned long enrollUniform(const Uniform& uniform) {
        uniforms.push_back(uniform);
        return uniforms.size() - 1;
    }

    void RenderScene(Scene* scene, Camera* camera) {
        // Clear existing commands
        _backend->ClearCommandQueue();
        

        
    }
};