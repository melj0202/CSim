#pragma once 
#include "Rendering/IBackend.h"
#include <GL/glew.h>
#include "Util/ArrayQueue.h"
#include "RenderCommand.h"
#include "GLDevice.h"
#include "Rendering/IRenderWindow.h"
#include "GLShaderProgram.h"
#include "GLTexture.h"
#include <unordered_map>


class GLBackend : public IBackend {
private:
    GLDevice* device;
    CommandQueue* commandQueue;
    IRenderWindow* window;
    int fps;

    std::unordered_map<unsigned long, std::unique_ptr<GLMesh>>> _vaoRegistryLookup;
    std::unordered_map<unsigned long, std::unique_ptr<GLShaderProgram>> _programRegistryLookup;
    std::unordered_map<unsigned long, std::unique_ptr<GLTexture>> _textureRegistryLookup;
    
    
public:
    GLBackend(IRenderWindow* window);
    ~GLBackend();
    
    void Initialize() override;
    void Shutdown() override;
    void BeginFrame() override;
    void EndFrame() override;
    void SubmitCommandQueue() override {
        device->ExecuteCommandQueue(*commandQueue);
    }
    void PushToCommandQueue(RenderCommand command) override {
        commandQueue->Submit(command);
    }
    int getFPS() const override { return fps; }
    unsigned long CreateMesh(const void* vertices, size_t vertexSize, const void* indices, size_t indexSize) override;
    unsigned long CreateMesh(std::string filePath) override;
    unsigned long CreateShaderProgram(const ShaderPaths& paths) override;
    unsigned long CreateShaderProgram(const ShaderSources& sources) override;
    unsigned long CreateTexture(const unsigned char* data, const int width, const int height) override;
    unsigned long CreateTexture(const std::string& filePath) override;  
    unsigned long CreateDescriptorSet() override { return 0; }
};