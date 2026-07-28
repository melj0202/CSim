#pragma once
#include "PipelineState.h"

// Opaque backend registry IDs (not raw GL object names). See D-R3.
// Data pointers in update payloads must remain valid until SubmitCommandQueue returns.

enum class CommandType {
	// Pipeline / raster state
	SetPipelineState,
	SetViewport,
	SetScissor,

	// Resource bindings (handles are table IDs)
	SetShader,
	SetMesh,
	SetVertexBuffer,
	SetIndexBuffer,
	SetTexture,
	SetUniformBuffer,

	// Uniforms
	SetUniformInt,
	SetUniformFloat,
	SetUniformVec2,
	SetUniformMat4,

	// Resource updates (same-frame pointer validity)
	UpdateTexture,
	UpdateBuffer,

	// Render targets / clear
	BeginRenderPass,
	ClearScreen,
	ClearDepthBuffer,
	ClearColorBuffer,
	ClearStencilBuffer,
	ClearAll,
	EndRenderPass,

	// Draw
	Draw,
	DrawIndexed,
	DrawInstanced,
	DrawBatched,
};

struct CmdClearColor {
	float r;
	float g;
	float b;
	float a;
};

struct CmdViewport {
	int x;
	int y;
	int width;
	int height;
};

struct CmdScissor {
	int x;
	int y;
	int width;
	int height;
};

struct CmdBindHandle {
	unsigned long handle;
	unsigned int slot;
};

struct CmdUniformInt {
	char name[32];
	int value;
};

struct CmdUniformFloat {
	char name[32];
	float value;
};

struct CmdUniformVec2 {
	char name[32];
	float x;
	float y;
};

struct CmdUniformMat4 {
	char name[32];
	float m[16];
};

struct CmdDraw {
	unsigned int elementCount;
	unsigned int first;
};

struct CmdDrawIndexed {
	unsigned int elementCount;
	unsigned int firstIndex;
};

struct CmdDrawInstanced {
	unsigned int elementCount;
	unsigned int instanceCount;
};

struct CmdUpdateTexture {
	unsigned long handle;
	int x;
	int y;
	int width;
	int height;
	// 0 = use texture's create format; otherwise channel count hint (3=RGB, 4=RGBA, 1=R)
	int channels;
	// Source buffer row length in *pixels*. 0 = tightly packed rows of `width`.
	// Use full texture width when `data` points into a larger staging buffer (dirty rects).
	int srcRowStride;
	const void* data;
};

struct CmdUpdateBuffer {
	unsigned long handle;
	unsigned int offsetBytes;
	unsigned int sizeBytes;
	const void* data;
};

// Tagged-union command token (D-R1). Trivially copyable for ArrayQueue.
struct RenderCommand {
	CommandType commandType = CommandType::ClearScreen;
	// Meaningful for SetPipelineState (and optionally read as current state seed).
	PipelineState pipelineState;

	union {
		CmdClearColor clear;
		CmdViewport viewport;
		CmdScissor scissor;
		CmdBindHandle bind;
		CmdUniformInt uniformInt;
		CmdUniformFloat uniformFloat;
		CmdUniformVec2 uniformVec2;
		CmdUniformMat4 uniformMat4;
		CmdDraw draw;
		CmdDrawIndexed drawIndexed;
		CmdDrawInstanced drawInstanced;
		CmdUpdateTexture updateTexture;
		CmdUpdateBuffer updateBuffer;
	};
};
