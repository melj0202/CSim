#pragma once
#include <string>
#include "CommandQueue.h"
#include "PipelineState.h"

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
    virtual int getFPS() const = 0;
    virtual unsigned long CreateBuffer(const void* vertices, size_t vertexSize, const void* indices, size_t indexSize) = 0;
    virtual unsigned long CreateShaderProgram(const std::string& vertexSource, const std::string& fragmentSource) = 0;
    virtual unsigned long CreateTexture(const unsigned char* data, const int width, const int height) = 0;
    virtual unsigned long CreateTexture(const std::string& filePath) = 0;  
    virtual unsigned long CreateDescriptorSet() = 0;
};