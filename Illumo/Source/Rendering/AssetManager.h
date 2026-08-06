#pragma once
#include "BackendConfig.h"
#include "IMesh.h"
#include "IShaderProgram.h"
#include "ITexture.h"
#include "PipelineState.h"
#include "Renderer.h"
#include "Services/EnvVars.h"
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

class AssetManager
{
  std::unordered_map<std::string, unsigned long> globalAssets;
  std::unordered_map<std::string, unsigned long> sceneAssets;
  unsigned long meshCount = 0;
  unsigned long textureCount = 0;
  unsigned long shaderCount = 0;
  EnvVars* envVars = nullptr;
  Renderer* renderer = nullptr;

  std::unordered_map<unsigned long, std::shared_ptr<IMesh>> meshRegistry;
  std::unordered_map<unsigned long, std::shared_ptr<ITexture>> textureRegistry;
  std::unordered_map<unsigned long, std::shared_ptr<IShaderProgram>>
    shaderRegistry;

public:
  AssetManager(Renderer* r)
    : renderer(r)
    , meshCount(0)
    , textureCount(0)
    , shaderCount(0)
  {
  }

  unsigned long LoadMeshToGlobal(const std::string& filePath)
  {
    globalAssets[filePath] = meshCount++;
    renderer->enrollMesh(filePath, meshCount - 1);
    return meshCount - 1;
  }

  unsigned long LoadMeshToScene(const std::string& filePath)
  {
    sceneAssets[filePath] = meshCount++;
    renderer->enrollMesh(filePath, meshCount - 1);
    return meshCount - 1;
  }

  unsigned long LoadMeshToGlobal(const std::vector<float>& data)
  {
    std::string key = "mesh_global_" + std::to_string(meshCount);
    globalAssets[key] = meshCount++;
    renderer->enrollMesh(
      data.data(), data.size() * sizeof(float), nullptr, 0, meshCount - 1);
    return meshCount - 1;
  }

  unsigned long LoadMeshToScene(const std::vector<float>& data)
  {
    std::string key = "mesh_scene_" + std::to_string(meshCount);
    sceneAssets[key] = meshCount++;
    renderer->enrollMesh(
      data.data(), data.size() * sizeof(float), nullptr, 0, meshCount - 1);
    return meshCount - 1;
  }

  unsigned long LoadTextureToGlobal(const std::string& filePath)
  {
    globalAssets[filePath] = textureCount++;
    renderer->enrollTexture(filePath, textureCount - 1);
    return textureCount - 1;
  }
  unsigned long LoadTextureToScene(const std::string& filePath)
  {
    sceneAssets[filePath] = textureCount++;
    renderer->enrollTexture(filePath, textureCount - 1);
    return textureCount - 1;
  }
  unsigned long LoadTextureToGlobal(const unsigned char* data,
                                    const int width,
                                    const int height)
  {
    std::string key = "tex_global_" + std::to_string(textureCount);
    globalAssets[key] = textureCount++;
    renderer->enrollTexture(data, width, height, textureCount - 1);
    return textureCount - 1;
  }

  unsigned long LoadTextureToScene(const unsigned char* data,
                                   const int width,
                                   const int height)
  {
    std::string key = "tex_scene_" + std::to_string(textureCount);
    sceneAssets[key] = textureCount++;
    renderer->enrollTexture(data, width, height, textureCount - 1);
    return textureCount - 1;
  }

  unsigned long LoadShaderToGlobal(const ShaderPaths& paths)
  {
    std::string key = paths.vertexPath + ";" + paths.fragmentPath;
    globalAssets[key] = shaderCount++;
    renderer->enrollShader(paths, shaderCount - 1);
    return shaderCount - 1;
  }

  unsigned long LoadShaderToScene(const ShaderPaths& paths)
  {
    std::string key = paths.vertexPath + ";" + paths.fragmentPath;
    sceneAssets[key] = shaderCount++;
    renderer->enrollShader(paths, shaderCount - 1);
    return shaderCount - 1;
  }

  unsigned long LoadShaderToGlobal(const ShaderSources& sources)
  {
    std::string key = "shader_sources_global_" + std::to_string(shaderCount);
    globalAssets[key] = shaderCount++;
    renderer->enrollShader(sources, shaderCount - 1);
    return shaderCount - 1;
  }

  unsigned long LoadShaderToScene(const ShaderSources& sources)
  {
    std::string key = "shader_sources_scene_" + std::to_string(shaderCount);
    sceneAssets[key] = shaderCount++;
    renderer->enrollShader(sources, shaderCount - 1);
    return shaderCount - 1;
  }

  std::shared_ptr<IShaderProgram> GetShader(unsigned long id)
  {
    return shaderRegistry[id];
  }

  std::shared_ptr<ITexture> GetTexture(unsigned long id)
  {
    return textureRegistry[id];
  }

  std::shared_ptr<IMesh> GetMesh(unsigned long id) { return meshRegistry[id]; }

  void Shutdown()
  {
    for (auto& mesh : meshRegistry) {
      if (mesh.second != nullptr) {
        mesh.second->Destroy();
      }
    }
    for (auto& texture : textureRegistry) {
      if (texture.second != nullptr) {
        texture.second->Destroy();
      }
    }
    for (auto& shader : shaderRegistry) {
      if (shader.second != nullptr) {
        shader.second->Destroy();
      }
    }
    meshRegistry.clear();
    textureRegistry.clear();
    shaderRegistry.clear();
    meshCount = 0;
    textureCount = 0;
    shaderCount = 0;
  }

  unsigned long GetShaderCount() { return shaderCount; }
  unsigned long GetMeshCount() { return meshCount; }
  unsigned long GetTextureCount() { return textureCount; }

  bool FreeShader(unsigned long id)
  {
    auto it = shaderRegistry.find(id);
    if (it != shaderRegistry.end()) {
      if (it->second != nullptr)
        it->second->Destroy();
      shaderRegistry.erase(it);
      return true;
    }
    return false;
  }

  bool FreeMesh(unsigned long id)
  {
    auto it = meshRegistry.find(id);
    if (it != meshRegistry.end()) {
      if (it->second != nullptr)
        it->second->Destroy();
      meshRegistry.erase(it);
      return true;
    }
    return false;
  }

  bool FreeTexture(unsigned long id)
  {
    auto it = textureRegistry.find(id);
    if (it != textureRegistry.end()) {
      if (it->second != nullptr)
        it->second->Destroy();
      textureRegistry.erase(it);
      return true;
    }
    return false;
  }
};