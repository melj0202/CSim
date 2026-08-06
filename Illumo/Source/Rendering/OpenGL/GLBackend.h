#pragma once
#include "Foundation/ArrayQueue.h"
#include "GLDevice.h"
#include "GLMesh.h"
#include "GLShaderProgram.h"
#include "GLTexture.h"
#include "RenderCommand.h"
#include "Rendering/IBackend.h"
#include "Rendering/IRenderWindow.h"
#include <GL/glew.h>
#include <memory>
#include <unordered_map>

class GLBackend : public IBackend
{
private:
  GLDevice* device;
  CommandQueue* commandQueue;
  IRenderWindow* window;
  int fps = 0;

  std::unordered_map<unsigned long, std::unique_ptr<GLMesh>> _vaoRegistryLookup;
  std::unordered_map<unsigned long, std::unique_ptr<GLShaderProgram>>
    _programRegistryLookup;
  std::unordered_map<unsigned long, std::unique_ptr<GLTexture>>
    _textureRegistryLookup;

public:
  GLBackend(IRenderWindow* window);
  ~GLBackend();

  void Initialize() override;
  void Shutdown() override;
  void BeginFrame() override;
  void EndFrame() override;
  void SubmitCommandQueue() override;
  void PushToCommandQueue(RenderCommand command) override;
  void ClearCommandQueue() override;
  int getFPS() const override { return fps; }

  unsigned long CreateMesh(const void* vertices,
                           size_t vertexSize,
                           const void* indices,
                           size_t indexSize,
                           unsigned long tableID) override;
  unsigned long CreateMesh(const void* vertices,
                           size_t vertexSize,
                           const void* indices,
                           size_t indexSize,
                           unsigned long tableID,
                           MeshVertexLayout layout,
                           bool dynamic) override;
  unsigned long CreateMesh(std::string filePath,
                           unsigned long tableID) override;
  unsigned long CreateShaderProgram(const ShaderPaths& paths,
                                    unsigned long tableID) override;
  unsigned long CreateShaderProgram(const ShaderSources& sources,
                                    unsigned long tableID) override;
  unsigned long CreateTexture(const unsigned char* data,
                              const int width,
                              const int height,
                              unsigned long tableID) override;
  unsigned long CreateTexture(const unsigned char* data,
                              const int width,
                              const int height,
                              int channels,
                              unsigned long tableID) override;
  unsigned long CreateTexture(const std::string& filePath,
                              unsigned long tableID) override;
  unsigned long CreateDescriptorSet() override { return 0; }
};
