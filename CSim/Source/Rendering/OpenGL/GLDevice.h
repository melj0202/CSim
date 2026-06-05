#pragma once
#include "GL/glew.h"
#include "Rendering/PipelineState.h"
#include "Rendering/CommandQueue.h"
#include "Rendering/HWInfo.h"
#include <unordered_map>
class GLDevice {
private:
    // Keeps track of the actual current hardware state to avoid redundant calls
    PipelineState _currentGLState;

    GLenum mapBlendFactor(BlendFactor factor) {
        switch (factor) {
            case BlendFactor::Zero:              return GL_ZERO;
            case BlendFactor::One:               return GL_ONE;
            case BlendFactor::SrcAlpha:          return GL_SRC_ALPHA;
            case BlendFactor::OneMinusSrcAlpha:  return GL_ONE_MINUS_SRC_ALPHA;
            case BlendFactor::SrcColor:          return GL_SRC_COLOR;
            case BlendFactor::OneMinusSrcColor:  return GL_ONE_MINUS_SRC_COLOR;
            default:                             return GL_ONE;
        }
    }

    GLenum mapCullMode(CullMode mode) {
        switch (mode) {
            case CullMode::Front:        return GL_FRONT;
            case CullMode::Back:         return GL_BACK;
            case CullMode::FrontAndBack: return GL_FRONT_AND_BACK;
            default:                     return GL_BACK;
        }
    }

    GLenum mapWindingOrder(WindingOrder order) {
        switch (order) {
            case WindingOrder::Clockwise:        return GL_CW;
            case WindingOrder::CounterClockwise: return GL_CCW;
            default:                             return GL_CCW;
        }
    }

    std::unordered_map<unsigned int, uint32_t> _vaoCache;
    std::unordered_map<unsigned int, uint32_t> _shaderCache;
    std::unordered_map<unsigned int, uint32_t> _textureCache;
    std::unordered_map<unsigned int, uint32_t> _descriptorSetCache;
    std::unordered_map<unsigned int, uint32_t> _framebufferCache;
    std::unordered_map<unsigned int, uint32_t> _uniformCache;

public:
    void ApplyPipelineState(const PipelineState& pipelineState);
    void ExecuteCommandQueue(CommandQueue& commandQueue);
    HWInfo GetHWInfo() {
        HWInfo info;
        info.gpuVendor = (const char*)glGetString(GL_VENDOR);
        info.gpuRenderer = (const char*)glGetString(GL_RENDERER);
        info.glVersion = (const char*)glGetString(GL_VERSION);
        info.glslVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.maxTextureSize);
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &info.maxTextureSlots);
        glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &info.maxUniformBlockSize);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &info.maxVertexAttributes);
        glGetIntegerv(GL_MAX_GEOMETRY_INPUT_COMPONENTS, &info.maxGeometryInputComponents);
        glGetIntegerv(GL_MAX_GEOMETRY_OUTPUT_COMPONENTS, &info.maxGeometryOutputComponents);
        glGetIntegerv(GL_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS, &info.maxGeometryTotalOutputComponents);
        glGetIntegerv(GL_MAX_TESS_CONTROL_INPUT_COMPONENTS, &info.maxTessControlInputComponents);
        glGetIntegerv(GL_MAX_TESS_CONTROL_OUTPUT_COMPONENTS, &info.maxTessControlOutputComponents);
        glGetIntegerv(GL_MAX_TESS_EVALUATION_INPUT_COMPONENTS, &info.maxTessEvaluationInputComponents);
        glGetIntegerv(GL_MAX_TESS_EVALUATION_OUTPUT_COMPONENTS, &info.maxTessEvaluationOutputComponents);

        // Check for advanced shader support
        const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
        
        if (extensions) {
            std::string extStr(extensions);
            
            // Geometry Shader
            info.hasGeometryShaderSupport = extStr.find("GL_ARB_geometry_shader4") != std::string::npos ||
                                        extStr.find("GL_EXT_geometry_shader") != std::string::npos;
            
            // Tessellation Shaders
            info.hasTessellationShaderSupport = extStr.find("GL_ARB_tessellation_shader") != std::string::npos;
            
            // Compute Shaders (Core in 4.3, but check for extension for broader compatibility)
            info.hasComputeShaderSupport = extStr.find("GL_ARB_compute_shader") != std::string::npos ||
                                       extStr.find("GL_VERSION_4_3") != std::string::npos;
            
            // Ray Tracing (Major extensions)
            info.hasRayTracingSupport = extStr.find("GL_NV_ray_tracing") != std::string::npos ||
                                     extStr.find("GL_KHR_ray_tracing") != std::string::npos ||
                                     extStr.find("GL_INTEL_performance_query") != std::string::npos;
            
            // Mesh Shaders
            info.hasMeshShaderSupport = extStr.find("GL_ARB_mesh_shader") != std::string::npos;
        }
        
        // Also check GL version for core features
        if (info.glVersion.find("4.0") != std::string::npos) {
            info.hasTessellationShaderSupport = true;
        }
        if (info.glVersion.find("4.3") != std::string::npos || info.glVersion.find("5") != std::string::npos) {
            info.hasComputeShaderSupport = true;
        }
        if (info.glVersion.find("4.6") != std::string::npos || info.glVersion.find("5") != std::string::npos) {
            info.hasMeshShaderSupport = true;

            
        }

        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &info.maxComputeWorkGroupInvocations);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_COUNT, info.maxComputeWorkGroupCount);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_SIZE, info.maxComputeWorkGroupSize);

        glGetIntegerv(GL_MAX_IMAGE_UNITS, &info.maxImageUnits);

        GLint totalMemoryKb = 0;
        glGetIntegerv(GL_GPU_MEM_INFO_TOTAL_AVAILABLE_MEM_NVX, &totalMemoryKb);
        if (totalMemoryKb > 0) {
            info.vramSize = totalMemoryKb / 1024;
        }

        // Query the maximum uniform buffer binding points
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &info.maxUniformBlocks);  
        return info;
    };
};
