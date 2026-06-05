#pragma once 
#include "PipelineState.h"

enum class CommandType {
    // --- 1. PIPELINE STATE ---
    SetPipelineState,    // Blending, Culling, Depth settings, Wireframe
    SetViewport,         // Resizing the window/rendering region
    SetScissor,

    // --- 2. RESOURCE BINDINGS ---
    SetShader,           // Activating a shader program
    SetVertexBuffer,     // Binding the vertex geometry data (or VAO in OpenGL)
    SetIndexBuffer,      // Binding the indices (EBO)
    SetTexture,          // Binding a texture to a specific slot (diffuse, normal, etc.)
    SetUniformBuffer,    // Binding constant data blocks (UBOs for matrices/lights)

    // --- 3. UNIFORMS (PUSH CONSTANTS / INSTANT DATA) ---
    SetUniformInt,       // Passing a quick integer to a shader
    SetUniformFloat,     // Passing a quick float to a shader
    SetUniformMat4,      // Passing a matrix (like World, View, Projection)

    // --- 4. RENDER TARGETS ---
    BeginRenderPass,     // Choosing a target Framebuffer (screen or offscreen texture)
    ClearScreen,         // Wiping the screen colors/depth to clear a frame
    ClearDepthBuffer,
    ClearColorBuffer,
    ClearStencilBuffer,
    ClearAll,
    EndRenderPass,       // Finishing drawing to the current target

    // --- 5. SUBMISSION / DRAW CALLS ---
    Draw,                // Raw sequential vertex draw (glDrawArrays)
    DrawIndexed,         // Standard index-buffered vertex draw (glDrawElements)
    DrawInstanced,       // For drawing thousands of identical meshes efficiently
    DrawBatched,
};

struct RenderCommand {
    unsigned long resourceHandle = 0;
    PipelineState pipelineState;
    unsigned int elementCount = 0;
    unsigned int slot = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    CommandType commandType;
};