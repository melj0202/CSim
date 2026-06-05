#pragma once
#include <unordered_map>
#include "PipelineState.h"
#include "Renderer.h"
#include <string>
#include <iostream>
#include <fstream>
#include "IShaderProgram.h"
#include "IMesh.h"
#include "ITexture.h"
#include "BackendConfig.h"
#include "System/EnvVars.h"


class AssetManager {
    std::unordered_map<std::string, unsigned long> globalAssets;
    std::unordered_map<std::string, unsigned long> sceneAssets;
    unsigned long meshCount;
    unsigned long textureCount;
    unsigned long shaderCount;
    EnvVars* envVars;


    AssetManager(EnvVars* ev)  {
        this->envVars = ev;
    }

    public:
    unsigned long LoadMeshToGlobal(const std::string& filePath) {
        globalAssets[filePath] = meshCount++;
        return meshCount-1;
    }

    unsigned long LoadMeshToScene(const std::string& filePath) {
        sceneAssets[filePath] = meshCount++;
        return meshCount-1;
    }

    unsigned long LoadMeshToGlobal(const std::vector<float> data&) {
        globalAssets[meshCount] = meshCount++;
        return meshCount-1;
    }

    unsigned long LoadMeshToScene(const std::vector<float> data&) {
        sceneAssets[meshCount] = meshCount++;
        return meshCount-1;
    }

    unsigned long LoadTextureToGlobal(const std::string& filePath) {
        globalAssets[filePath] = textureCount++;
        return textureCount-1;
    }
    unsigned long LoadTextureToScene(const std::string& filePath) {
        sceneAssets[filePath] = textureCount++;
        return textureCount-1;
    }
    unsigned long LoadTextureToGlobal(const char* data&, const int width, const int height) {
        globalAssets[textureCount] = textureCount++;
        return textureCount-1;
    }
    
    unsigned long LoadTextureToScene(const char* data&, const int width, const int height) {
        sceneAssets[textureCount] = textureCount++;
        return textureCount-1;
    }

    unsigned long LoadShaderToGlobal(const ShaderPaths& paths) {
        globalAssets[shaderCount] = shaderCount++;
        return shaderCount-1;
    }
    
    unsigned long LoadShaderToScene(const ShaderPaths& paths) {
        sceneAssets[shaderCount] = shaderCount++;
        return shaderCount-1;
    }
    
    unsigned long LoadShaderToGlobal(const char* source&) {
        globalAssets[shaderCount] =;
        return shaderCount-1;
    }
    
    unsigned long LoadShaderToScene(const char* source&) {
        sceneAssets[shaderCount] =;
        return shaderCount-1;
    }

    std::shared_ptr<IShader> GetShader(unsigned long id) {
        return shaderRegistry[id];
    }

    std::shared_ptr<ITexture> GetTexture(unsigned long id) {
        return textureRegistry[id];
    }

    std::shared_ptr<IMesh> GetMesh(unsigned long id) {
        return meshRegistry[id];
    }
    
    void Shutdown() {
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

    bool FreeShader(unsigned long id) {
        auto it = shaderRegistry.find(id);
        if (it != shaderRegistry.end()) {
            if (it->second != nullptr) it->second->Destroy();
            shaderRegistry.erase(it);
            return true;
        }
        return false;
    }
    
    bool FreeMesh(unsigned long id) {
        auto it = meshRegistry.find(id);
        if (it != meshRegistry.end()) {
            if (it->second != nullptr) it->second->Destroy();
            meshRegistry.erase(it);
            return true;
        }
        return false;
    }

    bool FreeTexture(unsigned long id) {
        auto it = textureRegistry.find(id);
        if (it != textureRegistry.end()) {
            if (it->second != nullptr) it->second->Destroy();
            textureRegistry.erase(it);
            return true;
        }
        return false;
    }
};