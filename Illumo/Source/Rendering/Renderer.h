#pragma once
#include "Camera.h"
#include "EnvVars.h"
#include "IBackend.h"
#include "IRenderWindow.h"
#include "IShaderProgram.h"
#include "RenderCommand.h"
#include "RenderPass.h"
#include "Scene.h"
#include <array>
#include <cstring>
#include <vector>

struct FrameBuffer
{
  unsigned int id = 0;
  unsigned int width = 0;
  unsigned int height = 0;
};

struct Uniform
{
  std::string name;
  unsigned int location = 0;
};

class Renderer
{
private:
  IBackend* _backend;
  bool _ownsBackend;
  IRenderWindow* _window;
  Camera* _camera;
  EnvVars* envVars;
  Scene* currentScene;
  std::vector<PipelineState> pipelineStates;
  std::vector<RenderPass> renderPasses;
  std::vector<FrameBuffer> frameBuffers;
  std::vector<Uniform> uniforms;

  // Phase 1 proof-quad resources (handles are table IDs)
  bool _proofReady = false;
  unsigned long _proofMeshHandle = 0;
  unsigned long _proofShaderHandle = 0;
  unsigned long _proofTextureHandle = 0;
  unsigned long _nextHandleId = 1;

  static void copyUniformName(char* dest, size_t destSize, const char* name)
  {
    if (!dest || destSize == 0) {
      return;
    }
    if (!name) {
      dest[0] = '\0';
      return;
    }
    size_t i = 0;
    for (; i + 1 < destSize && name[i] != '\0'; ++i) {
      dest[i] = name[i];
    }
    dest[i] = '\0';
  }

public:
  // Backend-neutral: operates only on IBackend*. Production composition
  // constructs the concrete backend (e.g. GLBackend via CreateOpenGLBackend)
  // and injects it with takeOwnership=true. Tests inject MockBackend with
  // takeOwnership=false so the caller's stack backend stays alive.
  Renderer(IRenderWindow* window,
           EnvVars* envVars,
           Camera* cam,
           IBackend* backend,
           bool takeOwnership)
    : _backend(backend)
    , _ownsBackend(takeOwnership)
    , _window(window)
    , _camera(cam)
    , envVars(envVars)
    , currentScene(nullptr)
  {
  }

  ~Renderer()
  {
    if (_backend && _ownsBackend) {
      _backend->Shutdown();
      delete _backend;
    }
    _backend = nullptr;
  }

  IBackend* getBackend() { return _backend; }
  const IBackend* getBackend() const { return _backend; }
  bool ownsBackend() const { return _ownsBackend; }
  IRenderWindow* getWindow() { return _window; }
  Camera* getCamera() { return _camera; }

  // =========================================================================
  // Asset enrollment (not mixed into the per-frame token stream — D-007)
  // =========================================================================

  unsigned long enrollShader(const ShaderPaths& paths, unsigned long tableID)
  {
    return _backend->CreateShaderProgram(paths, tableID);
  }

  unsigned long enrollShader(const ShaderSources& sources,
                             unsigned long tableID)
  {
    return _backend->CreateShaderProgram(sources, tableID);
  }

  unsigned long enrollMesh(const void* vertices,
                           const size_t verticesSize,
                           const void* indices,
                           const size_t indicesSize,
                           unsigned long tableID)
  {
    return _backend->CreateMesh(
      vertices, verticesSize, indices, indicesSize, tableID);
  }

  unsigned long enrollMesh(const void* vertices,
                           const size_t verticesSize,
                           const void* indices,
                           const size_t indicesSize,
                           unsigned long tableID,
                           MeshVertexLayout layout,
                           bool dynamic)
  {
    return _backend->CreateMesh(
      vertices, verticesSize, indices, indicesSize, tableID, layout, dynamic);
  }

  // Dynamic VBO (capacityBytes) + static index buffer; for UI text/console.
  unsigned long enrollDynamicMesh(size_t vertexCapacityBytes,
                                  const void* indices,
                                  size_t indicesSize,
                                  unsigned long tableID,
                                  MeshVertexLayout layout)
  {
    return _backend->CreateMesh(nullptr,
                                vertexCapacityBytes,
                                indices,
                                indicesSize,
                                tableID,
                                layout,
                                true);
  }

  unsigned long enrollMesh(std::string filePath, unsigned long tableID)
  {
    return _backend->CreateMesh(filePath, tableID);
  }

  unsigned long enrollTexture(const std::string& filePath,
                              unsigned long tableID)
  {
    return _backend->CreateTexture(filePath, tableID);
  }

  unsigned long enrollTexture(const unsigned char* data,
                              const int width,
                              const int height,
                              unsigned long tableID)
  {
    return _backend->CreateTexture(data, width, height, tableID);
  }

  unsigned long enrollTexture(const unsigned char* data,
                              const int width,
                              const int height,
                              int channels,
                              unsigned long tableID)
  {
    return _backend->CreateTexture(data, width, height, channels, tableID);
  }

  // Opaque table IDs for enroll* (v1: monotonic, never recycled).
  unsigned long allocateHandle() { return _nextHandleId++; }

  unsigned long enrollRenderPass(const RenderPass& renderPass)
  {
    renderPasses.push_back(renderPass);
    return renderPasses.size() - 1;
  }

  unsigned long enrollFrameBuffer(const FrameBuffer& frameBuffer)
  {
    frameBuffers.push_back(frameBuffer);
    return frameBuffers.size() - 1;
  }

  unsigned long enrollUniform(const Uniform& uniform)
  {
    uniforms.push_back(uniform);
    return uniforms.size() - 1;
  }

  // =========================================================================
  // Frame lifecycle
  // =========================================================================

  void BeginFrame()
  {
    _backend->BeginFrame();
    _backend->ClearCommandQueue();
  }

  void EndFrame()
  {
    // RenderScene usually already submitted; avoid a redundant empty walk.
    // (Backend still may receive an empty queue if callers only push then
    // EndFrame.)
    _backend->SubmitCommandQueue();
    _backend->EndFrame();
  }

  void SubmitOnly() { _backend->SubmitCommandQueue(); }

  // =========================================================================
  // Typed token helpers (push into backend queue)
  // =========================================================================

  void pushClearColor(float r, float g, float b, float a)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::ClearColorBuffer;
    cmd.clear.r = r;
    cmd.clear.g = g;
    cmd.clear.b = b;
    cmd.clear.a = a;
    _backend->PushToCommandQueue(cmd);
  }

  void pushClearScreen(float r, float g, float b, float a)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::ClearScreen;
    cmd.clear.r = r;
    cmd.clear.g = g;
    cmd.clear.b = b;
    cmd.clear.a = a;
    _backend->PushToCommandQueue(cmd);
  }

  void pushViewport(int x, int y, int width, int height)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::SetViewport;
    cmd.viewport.x = x;
    cmd.viewport.y = y;
    cmd.viewport.width = width;
    cmd.viewport.height = height;
    _backend->PushToCommandQueue(cmd);
  }

  void pushPipelineState(const PipelineState& state)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::SetPipelineState;
    cmd.pipelineState = state;
    _backend->PushToCommandQueue(cmd);
  }

  void pushSetShader(unsigned long handle)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::SetShader;
    cmd.bind.handle = handle;
    cmd.bind.slot = 0;
    _backend->PushToCommandQueue(cmd);
  }

  void pushSetMesh(unsigned long handle)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::SetMesh;
    cmd.bind.handle = handle;
    cmd.bind.slot = 0;
    _backend->PushToCommandQueue(cmd);
  }

  void pushSetTexture(unsigned long handle, unsigned int slot)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::SetTexture;
    cmd.bind.handle = handle;
    cmd.bind.slot = slot;
    _backend->PushToCommandQueue(cmd);
  }

  void pushUniformInt(const char* name, int value)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::SetUniformInt;
    copyUniformName(cmd.uniformInt.name, sizeof(cmd.uniformInt.name), name);
    cmd.uniformInt.value = value;
    _backend->PushToCommandQueue(cmd);
  }

  void pushUniformFloat(const char* name, float value)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::SetUniformFloat;
    copyUniformName(cmd.uniformFloat.name, sizeof(cmd.uniformFloat.name), name);
    cmd.uniformFloat.value = value;
    _backend->PushToCommandQueue(cmd);
  }

  void pushUniformVec2(const char* name, float x, float y)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::SetUniformVec2;
    copyUniformName(cmd.uniformVec2.name, sizeof(cmd.uniformVec2.name), name);
    cmd.uniformVec2.x = x;
    cmd.uniformVec2.y = y;
    _backend->PushToCommandQueue(cmd);
  }

  void pushUniformMat4(const char* name, const float* m16)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::SetUniformMat4;
    copyUniformName(cmd.uniformMat4.name, sizeof(cmd.uniformMat4.name), name);
    if (m16) {
      std::memcpy(cmd.uniformMat4.m, m16, 16 * sizeof(float));
    }
    _backend->PushToCommandQueue(cmd);
  }

  void pushDrawIndexed(unsigned int elementCount, unsigned int firstIndex = 0)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::DrawIndexed;
    cmd.drawIndexed.elementCount = elementCount;
    cmd.drawIndexed.firstIndex = firstIndex;
    _backend->PushToCommandQueue(cmd);
  }

  void pushUpdateTexture(unsigned long handle,
                         int x,
                         int y,
                         int width,
                         int height,
                         int channels,
                         const void* data,
                         int srcRowStride = 0)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::UpdateTexture;
    cmd.updateTexture.handle = handle;
    cmd.updateTexture.x = x;
    cmd.updateTexture.y = y;
    cmd.updateTexture.width = width;
    cmd.updateTexture.height = height;
    cmd.updateTexture.channels = channels;
    cmd.updateTexture.srcRowStride = srcRowStride;
    cmd.updateTexture.data = data;
    _backend->PushToCommandQueue(cmd);
  }

  void pushUpdateBuffer(unsigned long meshHandle,
                        unsigned int offsetBytes,
                        unsigned int sizeBytes,
                        const void* data)
  {
    RenderCommand cmd;
    cmd.commandType = CommandType::UpdateBuffer;
    cmd.updateBuffer.handle = meshHandle;
    cmd.updateBuffer.offsetBytes = offsetBytes;
    cmd.updateBuffer.sizeBytes = sizeBytes;
    cmd.updateBuffer.data = data;
    _backend->PushToCommandQueue(cmd);
  }

  // =========================================================================
  // Scene render (token-first; hybrid immediate only if AppendCommands fails)
  // Production: Canvas / CommandLine / GLString / SplashText are pure-token
  // (D-R10). Immediate Draw() remains for test stubs and any future unmigrated
  // drawable.
  // =========================================================================

  void RenderScene(Scene* scene, Camera* camera)
  {
    currentScene = scene;
    if (_camera == nullptr) {
      _camera = camera;
    }

    // Historical clear color (0.1, 0.1, 0.1).
    std::array<int, 2> dims = _window->getWindowDimensions();
    pushViewport(0, 0, dims[0], dims[1]);

    PipelineState defaultState;
    defaultState.depthTestEnabled = true;
    defaultState.blendEnabled = false;
    defaultState.faceCullingEnabled = false;
    defaultState.primitives = Primitives::Triangles;
    pushPipelineState(defaultState);

    pushClearScreen(0.1f, 0.1f, 0.1f, 1.0f);

    std::vector<DrawableBase*> immediateList;
    if (scene) {
      const std::vector<DrawableBase*>& list = scene->drawables;
      for (size_t i = 0; i < list.size(); ++i) {
        DrawableBase* drawable = list[i];
        if (!drawable) {
          continue;
        }
        // Pure-token: returns true. Immediate fallback if false (tests /
        // stubs).
        if (!drawable->AppendCommands(this)) {
          immediateList.push_back(drawable);
        }
      }
    }

    // Submit clear + token drawables before any immediate overlays.
    _backend->SubmitCommandQueue();
    _backend->ClearCommandQueue();

    for (size_t i = 0; i < immediateList.size(); ++i) {
      immediateList[i]->Draw();
    }
  }

  // =========================================================================
  // Phase 1: token proof path (env UseTokenProof=1)
  // =========================================================================

  void ensureProofResources()
  {
    if (_proofReady) {
      return;
    }

    // NDC-ish quad in pixel-ish space with identity MVP; shader multiplies by
    // uMVP. Layout: pos3 | color3 | uv2  (same as Canvas / triangle shaders)
    const float verts[32] = {
      1.0f, 1.0f, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f,  1.0f, 1.0f, -1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,  1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  1.0f,
      0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  0.0f, 0.0f, 1.0f,
    };
    const unsigned int indices[6] = { 0, 1, 2, 0, 2, 3 };

    _proofMeshHandle = allocateHandle();
    enrollMesh(
      verts, sizeof(verts), indices, sizeof(indices), _proofMeshHandle);

    ShaderPaths paths;
    paths.vertexPath = "Shader/triangle_vertex.glsl";
    paths.fragmentPath = "Shader/triangle_frag.glsl";
    _proofShaderHandle = allocateHandle();
    enrollShader(paths, _proofShaderHandle);

    // 2x2 RGBA checkerboard (magenta / dark)
    const int tw = 2;
    const int th = 2;
    unsigned char tex[2 * 2 * 4] = {
      255, 0, 255, 255, 40, 40, 40, 255, 40, 40, 40, 255, 255, 0, 255, 255,
    };
    _proofTextureHandle = allocateHandle();
    enrollTexture(tex, tw, th, 4, _proofTextureHandle);

    _proofReady = true;
  }

  // Clears + draws a fullscreen-ish textured quad entirely via tokens.
  // Caller must still swap buffers (or call EndFrame which submits+swaps).
  void RenderProofQuad()
  {
    ensureProofResources();

    _backend->ClearCommandQueue();

    std::array<int, 2> dims = _window->getWindowDimensions();
    pushViewport(0, 0, dims[0], dims[1]);

    PipelineState ps;
    ps.depthTestEnabled = false;
    ps.blendEnabled = false;
    ps.faceCullingEnabled = false;
    ps.primitives = Primitives::Triangles;
    pushPipelineState(ps);

    // Dark blue clear so we can tell token clear worked.
    pushClearScreen(0.05f, 0.08f, 0.18f, 1.0f);

    pushSetShader(_proofShaderHandle);
    pushSetMesh(_proofMeshHandle);
    pushSetTexture(_proofTextureHandle, 0);

    // Identity MVP → NDC quad fills clip space.
    float identity[16] = {
      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    };
    pushUniformMat4("uMVP", identity);
    pushUniformInt("ourTexture", 0);
    pushDrawIndexed(6, 0);

    _backend->SubmitCommandQueue();
    // Prevent EndFrame from re-executing the same tokens.
    _backend->ClearCommandQueue();
  }
};
