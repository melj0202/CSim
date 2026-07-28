#pragma once
#include <string>
#include "CommandQueue.h"
#include "PipelineState.h"
#include "IShaderProgram.h"

class IBackend {
public:
    IBackend() = default;
    virtual ~IBackend() = default;

    virtual void Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void SubmitCommandQueue() = 0;
    virtual void PushToCommandQueue(RenderCommand command) = 0;
    virtual void ClearCommandQueue() = 0;
    virtual int getFPS() const = 0;
    
    virtual unsigned long CreateMesh(const void* vertices, size_t vertexSize, const void* indices, size_t indexSize, unsigned long tableID) = 0;
    virtual unsigned long CreateMesh(std::string filePath, unsigned long tableID) = 0;
    virtual unsigned long CreateShaderProgram(const ShaderPaths& paths, unsigned long tableID) = 0;
    virtual unsigned long CreateShaderProgram(const ShaderSources& sources, unsigned long tableID) = 0;
    virtual unsigned long CreateTexture(const unsigned char* data, const int width, const int height, unsigned long tableID) = 0;
    virtual unsigned long CreateTexture(const std::string& filePath, unsigned long tableID) = 0;  
    virtual unsigned long CreateDescriptorSet() = 0;
};