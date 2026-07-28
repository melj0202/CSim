#include "Canvas.h"
#include "IRenderWindow.h"
#include "Logger.h"
#include "Rendering/Camera.h"
#include "Rendering/Renderer.h"
#include "Rendering/IShaderProgram.h"
#include <array>
#include <cmath>
#include <cstring>
#include <glm/fwd.hpp>

Canvas::Canvas(int width, int height, IRenderWindow* window, Camera* camera, Renderer* renderer)
{
	this->window = window;
	this->camera = camera;
	this->renderer = renderer;
	lifeCanvas = nullptr;
	texCanvasBuffer = nullptr;
	displayRgb = nullptr;
	targetRgb = nullptr;
	fadeSpeed = 8.0f;
	meshHandle = 0;
	shaderHandle = 0;
	textureHandle = 0;
	gpuReady = false;
	canvasWidth = 0;
	canvasHeight = 0;
	initCanvas(width, height);
}

Canvas::~Canvas()
{
	freeCanvas();
}

void Canvas::initCanvas(const int& width, const int& height)
{
	canvasWidth = width;
	canvasHeight = height;
	fadeSpeed = 8.0f;

	lifeCanvas = new unsigned char[static_cast<size_t>(width * height)];
	memset(lifeCanvas, 1, static_cast<size_t>(width * height));

	texCanvasBuffer = new unsigned char[static_cast<size_t>(width * height * 3)];
	memset(texCanvasBuffer, 255, static_cast<size_t>(width * height * 3));

	const int rgbCount = width * height * 3;
	displayRgb = new float[static_cast<size_t>(rgbCount)];
	targetRgb = new float[static_cast<size_t>(rgbCount)];
	for (int i = 0; i < rgbCount; ++i)
	{
		displayRgb[i] = 1.0f;
		targetRgb[i] = 1.0f;
	}

	const float cellSize = 16.0f;
	const float worldW = static_cast<float>(width) * cellSize;
	const float worldH = static_cast<float>(height) * cellSize;
	// pos3 | color3 | uv2 — matches triangle_vertex.glsl / GLMesh layout
	vertices = {
		worldW, worldH, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
		worldW, 0.0f,   0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
		0.0f,   0.0f,   0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
		0.0f,   worldH, 0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,
	};
	indices = { 1, 2, 3, 0, 1, 3 };

	enrollGpuResources();
	Logger::LogTrace("Canvas initialized (token / enroll path)");
}

void Canvas::enrollGpuResources()
{
	gpuReady = false;
	if (!renderer)
	{
		Logger::LogError("Canvas: no Renderer — cannot enroll GPU resources");
		return;
	}

	meshHandle = renderer->allocateHandle();
	renderer->enrollMesh(
		vertices.data(),
		vertices.size() * sizeof(float),
		indices.data(),
		indices.size() * sizeof(unsigned int),
		meshHandle);

	ShaderPaths paths;
	paths.vertexPath = "Shader/triangle_vertex.glsl";
	paths.fragmentPath = "Shader/triangle_frag.glsl";
	shaderHandle = renderer->allocateHandle();
	renderer->enrollShader(paths, shaderHandle);

	// Initial white RGB grid (logical lifeCanvas is 0/1 flags; tex is RGB display).
	textureHandle = renderer->allocateHandle();
	renderer->enrollTexture(
		texCanvasBuffer,
		canvasWidth,
		canvasHeight,
		3,
		textureHandle);

	gpuReady = true;
}

void Canvas::freeCanvas()
{
	delete[] texCanvasBuffer;
	delete[] lifeCanvas;
	delete[] displayRgb;
	delete[] targetRgb;
	texCanvasBuffer = nullptr;
	lifeCanvas = nullptr;
	displayRgb = nullptr;
	targetRgb = nullptr;
	// GPU resources: destroy-only-at-shutdown for v1 (D-R4). Handles left in backend registry.
	gpuReady = false;
}

void Canvas::setTargetColor(int cellIndex, unsigned char r, unsigned char g, unsigned char b)
{
	const int base = cellIndex * 3;
	targetRgb[base + 0] = static_cast<float>(r) / 255.0f;
	targetRgb[base + 1] = static_cast<float>(g) / 255.0f;
	targetRgb[base + 2] = static_cast<float>(b) / 255.0f;
}

void Canvas::snapVisualToTargets()
{
	const int rgbCount = canvasWidth * canvasHeight * 3;
	for (int i = 0; i < rgbCount; ++i)
	{
		displayRgb[i] = targetRgb[i];
		const float v = displayRgb[i] * 255.0f + 0.5f;
		texCanvasBuffer[i] = static_cast<unsigned char>(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
	}
}

void Canvas::tickVisual(float dt)
{
	if (dt < 0.0f)
	{
		dt = 0.0f;
	}
	float alpha = 1.0f - expf(-fadeSpeed * dt);
	if (alpha > 1.0f)
	{
		alpha = 1.0f;
	}
	if (fadeSpeed <= 0.0f)
	{
		alpha = 1.0f;
	}

	const int rgbCount = canvasWidth * canvasHeight * 3;
	for (int i = 0; i < rgbCount; ++i)
	{
		displayRgb[i] = displayRgb[i] + (targetRgb[i] - displayRgb[i]) * alpha;
		const float v = displayRgb[i] * 255.0f + 0.5f;
		texCanvasBuffer[i] = static_cast<unsigned char>(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
	}
}

void Canvas::DrawImpl()
{
	// Migrated to tokens; RenderScene should not call Draw() for Canvas.
}

bool Canvas::AppendCommands(Renderer* r)
{
	if (!isVisible() || !gpuReady || !r)
	{
		// Invisible: skip immediate path too. Not ready: fall back if possible.
		if (!isVisible())
		{
			return true;
		}
		return false;
	}

	// Full texture rewrite each frame (D-R4 v1).
	r->pushUpdateTexture(
		textureHandle,
		0,
		0,
		canvasWidth,
		canvasHeight,
		3,
		texCanvasBuffer);

	PipelineState ps;
	ps.depthTestEnabled = false;
	ps.blendEnabled = false;
	ps.faceCullingEnabled = false;
	ps.primitives = Primitives::Triangles;
	r->pushPipelineState(ps);

	r->pushSetShader(shaderHandle);
	r->pushSetMesh(meshHandle);
	r->pushSetTexture(textureHandle, 0);

	std::array<int, 2> dims = window->getWindowDimensions();
	float aspect = static_cast<float>(dims[0]) / static_cast<float>(dims[1]);
	glm::mat4 mvp = camera->GetMVPMatrix(aspect);
	r->pushUniformMat4("uMVP", &mvp[0][0]);
	r->pushUniformInt("ourTexture", 0);
	r->pushDrawIndexed(6, 0);

	return true;
}
