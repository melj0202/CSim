// Phase 6: headless token/backend tests (no OpenGL, no window).
// Exit 0 on success; non-zero on failure. Run via CTest target CSimRenderTests.

#include "Rendering/Mock/MockBackend.h"
#include "Rendering/RenderCommand.h"
#include "Rendering/PipelineState.h"
#include "Rendering/IMesh.h"
#include "Rendering/IShaderProgram.h"
#include <cstdio>
#include <cstring>

static int g_failures = 0;

static void expectTrue(bool cond, const char* msg)
{
	if (!cond)
	{
		std::printf("FAIL: %s\n", msg);
		++g_failures;
	}
	else
	{
		std::printf("PASS: %s\n", msg);
	}
}

static void expectEqSize(size_t a, size_t b, const char* msg)
{
	if (a != b)
	{
		std::printf("FAIL: %s (got %zu, expected %zu)\n", msg, a, b);
		++g_failures;
	}
	else
	{
		std::printf("PASS: %s\n", msg);
	}
}

static void expectEqInt(int a, int b, const char* msg)
{
	if (a != b)
	{
		std::printf("FAIL: %s (got %d, expected %d)\n", msg, a, b);
		++g_failures;
	}
	else
	{
		std::printf("PASS: %s\n", msg);
	}
}

static void expectCommandType(CommandType got, CommandType expected, const char* msg)
{
	if (got != expected)
	{
		std::printf("FAIL: %s (command type mismatch)\n", msg);
		++g_failures;
	}
	else
	{
		std::printf("PASS: %s\n", msg);
	}
}

// Mirrors the production proof-quad / frame-setup vocabulary without Renderer/GL.
static void emitProofLikeFrame(MockBackend& backend, unsigned long shaderH, unsigned long meshH, unsigned long texH)
{
	backend.ClearCommandQueue();

	RenderCommand vp;
	vp.commandType = CommandType::SetViewport;
	vp.viewport.x = 0;
	vp.viewport.y = 0;
	vp.viewport.width = 1280;
	vp.viewport.height = 720;
	backend.PushToCommandQueue(vp);

	RenderCommand pipe;
	pipe.commandType = CommandType::SetPipelineState;
	pipe.pipelineState.depthTestEnabled = false;
	pipe.pipelineState.blendEnabled = false;
	pipe.pipelineState.primitives = Primitives::Triangles;
	backend.PushToCommandQueue(pipe);

	RenderCommand clear;
	clear.commandType = CommandType::ClearScreen;
	clear.clear.r = 0.05f;
	clear.clear.g = 0.08f;
	clear.clear.b = 0.18f;
	clear.clear.a = 1.0f;
	backend.PushToCommandQueue(clear);

	RenderCommand setShader;
	setShader.commandType = CommandType::SetShader;
	setShader.bind.handle = shaderH;
	setShader.bind.slot = 0;
	backend.PushToCommandQueue(setShader);

	RenderCommand setMesh;
	setMesh.commandType = CommandType::SetMesh;
	setMesh.bind.handle = meshH;
	backend.PushToCommandQueue(setMesh);

	RenderCommand setTex;
	setTex.commandType = CommandType::SetTexture;
	setTex.bind.handle = texH;
	setTex.bind.slot = 0;
	backend.PushToCommandQueue(setTex);

	RenderCommand mat;
	mat.commandType = CommandType::SetUniformMat4;
	std::memset(mat.uniformMat4.name, 0, sizeof(mat.uniformMat4.name));
	std::memcpy(mat.uniformMat4.name, "uMVP", 4);
	// identity
	for (int i = 0; i < 16; ++i)
	{
		mat.uniformMat4.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
	}
	backend.PushToCommandQueue(mat);

	RenderCommand samp;
	samp.commandType = CommandType::SetUniformInt;
	std::memset(samp.uniformInt.name, 0, sizeof(samp.uniformInt.name));
	std::memcpy(samp.uniformInt.name, "ourTexture", 10);
	samp.uniformInt.value = 0;
	backend.PushToCommandQueue(samp);

	RenderCommand draw;
	draw.commandType = CommandType::DrawIndexed;
	draw.drawIndexed.elementCount = 6;
	draw.drawIndexed.firstIndex = 0;
	backend.PushToCommandQueue(draw);

	backend.SubmitCommandQueue();
}

static void testLifecycle()
{
	std::printf("\n--- lifecycle ---\n");
	MockBackend mock;
	expectTrue(!mock.wasInitialized(), "starts uninitialized");
	mock.Initialize();
	expectTrue(mock.wasInitialized(), "Initialize sets flag");
	mock.BeginFrame();
	mock.EndFrame();
	expectEqInt(mock.getBeginFrameCount(), 1, "BeginFrame counted");
	expectEqInt(mock.getEndFrameCount(), 1, "EndFrame counted");
	mock.Shutdown();
	expectTrue(mock.wasShutdown(), "Shutdown sets flag");
}

static void testCreateRecords()
{
	std::printf("\n--- create / enroll records ---\n");
	MockBackend mock;
	mock.Initialize();

	float verts[8] = {0};
	unsigned int idx[6] = {0, 1, 2, 0, 2, 3};
	unsigned long meshId = mock.CreateMesh(verts, sizeof(verts), idx, sizeof(idx), 10);
	expectEqSize(static_cast<size_t>(meshId), 10u, "CreateMesh returns tableID");

	unsigned long dynId = mock.CreateMesh(nullptr, 4096, idx, sizeof(idx), 11, MeshVertexLayout::Pos3Color4U8, true);
	expectEqSize(static_cast<size_t>(dynId), 11u, "CreateMesh dynamic returns tableID");

	ShaderPaths paths;
	paths.vertexPath = "v.glsl";
	paths.fragmentPath = "f.glsl";
	unsigned long shId = mock.CreateShaderProgram(paths, 20);
	expectEqSize(static_cast<size_t>(shId), 20u, "CreateShaderProgram paths returns tableID");

	unsigned char px[4] = {255, 0, 255, 255};
	unsigned long texId = mock.CreateTexture(px, 1, 1, 4, 30);
	expectEqSize(static_cast<size_t>(texId), 30u, "CreateTexture returns tableID");

	expectEqSize(mock.getCreateCount(), 4u, "four create records");
	expectTrue(mock.getCreate(1).dynamic == true, "dynamic mesh flagged");
	expectTrue(mock.getCreate(1).layout == MeshVertexLayout::Pos3Color4U8, "UI layout recorded");
	expectEqInt(mock.getCreate(3).channels, 4, "texture channels recorded");
}

static void testCommandSubmit()
{
	std::printf("\n--- command queue submit snapshot ---\n");
	MockBackend mock;
	mock.Initialize();

	RenderCommand a;
	a.commandType = CommandType::ClearColorBuffer;
	a.clear.r = 0.1f;
	a.clear.g = 0.1f;
	a.clear.b = 0.1f;
	a.clear.a = 1.0f;
	mock.PushToCommandQueue(a);

	RenderCommand b;
	b.commandType = CommandType::SetViewport;
	b.viewport.width = 100;
	b.viewport.height = 50;
	mock.PushToCommandQueue(b);

	expectEqSize(mock.getPendingCommandCount(), 2u, "two pending before submit");
	mock.SubmitCommandQueue();
	expectEqInt(mock.getSubmitCount(), 1, "submit counted");
	expectEqSize(mock.getLastSubmittedCount(), 2u, "snapshot has two commands");
	expectCommandType(mock.getLastSubmittedType(0), CommandType::ClearColorBuffer, "first is clear");
	expectCommandType(mock.getLastSubmittedType(1), CommandType::SetViewport, "second is viewport");
	expectEqInt(mock.getLastSubmitted(1).viewport.width, 100, "viewport width preserved");
	expectEqInt(mock.getLastSubmitted(1).viewport.height, 50, "viewport height preserved");

	mock.ClearCommandQueue();
	expectEqSize(mock.getPendingCommandCount(), 0u, "clear empties pending");
	// Last submitted snapshot remains until next submit
	expectEqSize(mock.getLastSubmittedCount(), 2u, "clear does not wipe last snapshot");
}

static void testProofLikeSequence()
{
	std::printf("\n--- proof-like token sequence ---\n");
	MockBackend mock;
	mock.Initialize();

	const unsigned long meshH = mock.CreateMesh(nullptr, 128, nullptr, 0, 1, MeshVertexLayout::Pos3Color3Uv2, false);
	ShaderSources src;
	src.vertexSource = "void main(){}";
	src.fragmentSource = "void main(){}";
	const unsigned long shaderH = mock.CreateShaderProgram(src, 2);
	const unsigned long texH = mock.CreateTexture(nullptr, 2, 2, 4, 3);

	mock.BeginFrame();
	emitProofLikeFrame(mock, shaderH, meshH, texH);
	mock.EndFrame();

	expectEqInt(mock.getSubmitCount(), 1, "one submit for proof frame");
	expectEqSize(mock.getLastSubmittedCount(), 9u, "nine tokens in proof-like frame");

	const CommandType expected[] = {
		CommandType::SetViewport,
		CommandType::SetPipelineState,
		CommandType::ClearScreen,
		CommandType::SetShader,
		CommandType::SetMesh,
		CommandType::SetTexture,
		CommandType::SetUniformMat4,
		CommandType::SetUniformInt,
		CommandType::DrawIndexed,
	};
	expectTrue(mock.submittedStartsWith(expected, 9), "proof sequence order matches");
	expectEqSize(mock.countSubmittedOfType(CommandType::DrawIndexed), 1u, "one DrawIndexed");
	expectEqSize(mock.countSubmittedOfType(CommandType::ClearScreen), 1u, "one ClearScreen");

	// Handles in bind tokens should be opaque table IDs we enrolled
	expectTrue(mock.getLastSubmitted(3).bind.handle == shaderH, "SetShader handle is enrolled ID");
	expectTrue(mock.getLastSubmitted(4).bind.handle == meshH, "SetMesh handle is enrolled ID");
	expectTrue(mock.getLastSubmitted(5).bind.handle == texH, "SetTexture handle is enrolled ID");
	expectEqInt(static_cast<int>(mock.getLastSubmitted(8).drawIndexed.elementCount), 6, "DrawIndexed elementCount 6");
}

static void testCanvasLikeUpdateTexture()
{
	std::printf("\n--- canvas-like UpdateTexture token ---\n");
	MockBackend mock;
	mock.Initialize();
	unsigned long texH = mock.CreateTexture(nullptr, 80, 60, 3, 5);

	unsigned char fakePixels[12] = {0};
	RenderCommand upd;
	upd.commandType = CommandType::UpdateTexture;
	upd.updateTexture.handle = texH;
	upd.updateTexture.x = 0;
	upd.updateTexture.y = 0;
	upd.updateTexture.width = 80;
	upd.updateTexture.height = 60;
	upd.updateTexture.channels = 3;
	upd.updateTexture.data = fakePixels;
	mock.PushToCommandQueue(upd);

	RenderCommand draw;
	draw.commandType = CommandType::DrawIndexed;
	draw.drawIndexed.elementCount = 6;
	mock.PushToCommandQueue(draw);
	mock.SubmitCommandQueue();

	expectEqSize(mock.countSubmittedOfType(CommandType::UpdateTexture), 1u, "UpdateTexture recorded");
	expectTrue(mock.getLastSubmitted(0).updateTexture.handle == texH, "UpdateTexture handle matches");
	expectEqInt(mock.getLastSubmitted(0).updateTexture.channels, 3, "RGB channels on update");
	expectTrue(mock.getLastSubmitted(0).updateTexture.data == fakePixels, "data pointer preserved until submit");
}

int runMockBackendTests()
{
	g_failures = 0;
	std::printf("\n======== MockBackend unit tests ========\n");
	testLifecycle();
	testCreateRecords();
	testCommandSubmit();
	testProofLikeSequence();
	testCanvasLikeUpdateTexture();
	std::printf("======== MockBackend done (%d failure(s)) ========\n", g_failures);
	return g_failures;
}
