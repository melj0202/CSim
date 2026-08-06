#pragma once
#include "GL/glew.h"
#ifndef GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX
#define GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX 0x9048
#endif
#include "GLMesh.h"
#include "GLShaderProgram.h"
#include "GLTexture.h"
#include "Rendering/CommandQueue.h"
#include "Rendering/HWInfo.h"
#include "Rendering/PipelineState.h"
#include <memory>
#include <string>
#include <unordered_map>

// Registries owned by GLBackend; opaque handles are keys into these maps.
struct GLResourceTables
{
  const std::unordered_map<unsigned long, std::unique_ptr<GLMesh>>* meshes =
    nullptr;
  const std::unordered_map<unsigned long, std::unique_ptr<GLShaderProgram>>*
    programs = nullptr;
  const std::unordered_map<unsigned long, std::unique_ptr<GLTexture>>*
    textures = nullptr;
};

class GLDevice
{
private:
  PipelineState _currentGLState;
  GLuint _activeProgram = 0;

  // Bind-state tracker (P4): skip redundant GL binds within a submit.
  GLuint _boundProgram = 0;
  GLuint _boundVao = 0;
  GLuint _boundTexture[8] = {};
  int _viewportX = -1;
  int _viewportY = -1;
  int _viewportW = -1;
  int _viewportH = -1;

  // Cache: key is "progId:name"
  std::unordered_map<std::string, GLint> _uniformLocationCache;

  GLenum mapBlendFactor(BlendFactor factor)
  {
    switch (factor) {
      case BlendFactor::Zero:
        return GL_ZERO;
      case BlendFactor::One:
        return GL_ONE;
      case BlendFactor::SrcAlpha:
        return GL_SRC_ALPHA;
      case BlendFactor::OneMinusSrcAlpha:
        return GL_ONE_MINUS_SRC_ALPHA;
      case BlendFactor::SrcColor:
        return GL_SRC_COLOR;
      case BlendFactor::OneMinusSrcColor:
        return GL_ONE_MINUS_SRC_COLOR;
      default:
        return GL_ONE;
    }
  }

  GLenum mapCullMode(CullMode mode)
  {
    switch (mode) {
      case CullMode::Front:
        return GL_FRONT;
      case CullMode::Back:
        return GL_BACK;
      case CullMode::FrontAndBack:
        return GL_FRONT_AND_BACK;
      default:
        return GL_BACK;
    }
  }

  GLenum mapWindingOrder(WindingOrder order)
  {
    switch (order) {
      case WindingOrder::Clockwise:
        return GL_CW;
      case WindingOrder::CounterClockwise:
        return GL_CCW;
      default:
        return GL_CCW;
    }
  }

  GLenum mapPrimitives(Primitives p)
  {
    switch (p) {
      case Primitives::Points:
        return GL_POINTS;
      case Primitives::Lines:
        return GL_LINES;
      case Primitives::Triangles:
        return GL_TRIANGLES;
      default:
        return GL_TRIANGLES;
    }
  }

  GLint getUniformLocation(const char* name)
  {
    if (_activeProgram == 0 || name == nullptr) {
      return -1;
    }
    std::string key = std::to_string(_activeProgram);
    key.push_back(':');
    key.append(name);
    std::unordered_map<std::string, GLint>::iterator it =
      _uniformLocationCache.find(key);
    if (it != _uniformLocationCache.end()) {
      return it->second;
    }
    GLint loc = glGetUniformLocation(_activeProgram, name);
    _uniformLocationCache[key] = loc;
    return loc;
  }

  GLMesh* resolveMesh(const GLResourceTables& tables,
                      unsigned long handle) const
  {
    if (!tables.meshes) {
      return nullptr;
    }
    std::unordered_map<unsigned long, std::unique_ptr<GLMesh>>::const_iterator
      it = tables.meshes->find(handle);
    if (it == tables.meshes->end()) {
      return nullptr;
    }
    return it->second.get();
  }

  GLShaderProgram* resolveProgram(const GLResourceTables& tables,
                                  unsigned long handle) const
  {
    if (!tables.programs) {
      return nullptr;
    }
    std::unordered_map<unsigned long,
                       std::unique_ptr<GLShaderProgram>>::const_iterator it =
      tables.programs->find(handle);
    if (it == tables.programs->end()) {
      return nullptr;
    }
    return it->second.get();
  }

  GLTexture* resolveTexture(const GLResourceTables& tables,
                            unsigned long handle) const
  {
    if (!tables.textures) {
      return nullptr;
    }
    std::unordered_map<unsigned long,
                       std::unique_ptr<GLTexture>>::const_iterator it =
      tables.textures->find(handle);
    if (it == tables.textures->end()) {
      return nullptr;
    }
    return it->second.get();
  }

public:
  void ApplyPipelineState(const PipelineState& pipelineState);
  void ExecuteCommandQueue(CommandQueue& commandQueue,
                           const GLResourceTables& tables);
  HWInfo GetHWInfo();
};
