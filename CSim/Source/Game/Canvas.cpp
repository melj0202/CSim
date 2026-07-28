#include "Canvas.h"
#include "IRenderWindow.h"
#include "Logger.h"
#include "Rendering/Camera.h"
#include "Rendering/Renderer.h"
#include "Rendering/IShaderProgram.h"
#include "Rulesets/RuleSet.h"
#include <array>
#include <cstring>
#include <glm/fwd.hpp>
#include <tracy/Tracy.hpp>

Canvas::Canvas(int width, int height, IRenderWindow* window, Camera* camera, Renderer* renderer)
{
	this->window = window;
	this->camera = camera;
	this->renderer = renderer;
	lifeCanvas = nullptr;
	fadeSpeed = 8.0f;
	meshHandle = 0;
	shaderHandle = 0;
	cellTextureHandle = 0;
	paletteTextureHandle = 0;
	gpuReady = false;
	canvasWidth = 0;
	canvasHeight = 0;
	cellsDirty = true;
	textureUploadPending = true;
	paletteUploadPending = true;
	cellsDirtyRect.clear();
	uploadDirtyRect.clear();
	std::memset(paletteRgb, 255, sizeof(paletteRgb));
	initCanvas(width, height);
}

Canvas::~Canvas()
{
	freeCanvas();
}

void Canvas::rebuildDefaultPalette()
{
	// Project convention: 0 = alive (black), 1 = dead (white); other states mid-gray.
	for (int s = 0; s < kPaletteSize; ++s)
	{
		const int base = s * 3;
		if (s == 0)
		{
			paletteRgb[base + 0] = 0;
			paletteRgb[base + 1] = 0;
			paletteRgb[base + 2] = 0;
		}
		else if (s == 1)
		{
			paletteRgb[base + 0] = 255;
			paletteRgb[base + 1] = 255;
			paletteRgb[base + 2] = 255;
		}
		else
		{
			// Distinct but muted default for multi-state rules before rebuildPalette.
			paletteRgb[base + 0] = 0;
			paletteRgb[base + 1] = 164;
			paletteRgb[base + 2] = 128;
		}
	}
	paletteUploadPending = true;
}

void Canvas::rebuildPalette(const RuleSet* rules)
{
	ZoneScopedN("Canvas.rebuildPalette");
	if (!rules)
	{
		rebuildDefaultPalette();
		return;
	}
	for (int s = 0; s < kPaletteSize; ++s)
	{
		unsigned char rgb[3] = {255, 255, 255};
		rules->evalCell(static_cast<unsigned char>(s), rgb);
		const int base = s * 3;
		paletteRgb[base + 0] = rgb[0];
		paletteRgb[base + 1] = rgb[1];
		paletteRgb[base + 2] = rgb[2];
	}
	paletteUploadPending = true;
}

void Canvas::initCanvas(const int& width, const int& height)
{
	canvasWidth = width;
	canvasHeight = height;
	fadeSpeed = 8.0f;

	lifeCanvas = new unsigned char[static_cast<size_t>(width * height)];
	memset(lifeCanvas, 1, static_cast<size_t>(width * height));

	rebuildDefaultPalette();

	const float cellSize = 16.0f;
	const float worldW = static_cast<float>(width) * cellSize;
	const float worldH = static_cast<float>(height) * cellSize;
	vertices = {
		worldW, worldH, 0.0f,  1.0f, 0.0f, 0.0f,  1.0f, 1.0f,
		worldW, 0.0f,   0.0f,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
		0.0f,   0.0f,   0.0f,  0.0f, 0.0f, 1.0f,  0.0f, 0.0f,
		0.0f,   worldH, 0.0f,  1.0f, 1.0f, 0.0f,  0.0f, 1.0f,
	};
	indices = { 1, 2, 3, 0, 1, 3 };

	cellsDirty = true;
	cellsDirtyRect.setFull(width, height);
	textureUploadPending = true;
	uploadDirtyRect.setFull(width, height);

	enrollGpuResources();
	Logger::LogTrace("Canvas initialized (R8 + palette token path)");
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
	paths.vertexPath = "Shader/canvas_vertex.glsl";
	paths.fragmentPath = "Shader/canvas_frag.glsl";
	shaderHandle = renderer->allocateHandle();
	renderer->enrollShader(paths, shaderHandle);

	// R8 cell-state texture (1 byte/cell).
	cellTextureHandle = renderer->allocateHandle();
	renderer->enrollTexture(
		lifeCanvas,
		canvasWidth,
		canvasHeight,
		1,
		cellTextureHandle);

	// 256×1 RGB palette.
	paletteTextureHandle = renderer->allocateHandle();
	renderer->enrollTexture(
		paletteRgb,
		kPaletteSize,
		1,
		3,
		paletteTextureHandle);

	gpuReady = true;
	textureUploadPending = true;
	uploadDirtyRect.setFull(canvasWidth, canvasHeight);
	paletteUploadPending = true;
}

void Canvas::freeCanvas()
{
	delete[] lifeCanvas;
	lifeCanvas = nullptr;
	gpuReady = false;
}

void Canvas::noteUploadRegion(int x0, int y0, int x1, int y1)
{
	textureUploadPending = true;
	uploadDirtyRect.includeRect(x0, y0, x1, y1);
}

void Canvas::markCellsDirty()
{
	cellsDirty = true;
	cellsDirtyRect.setFull(canvasWidth, canvasHeight);
	noteUploadRegion(0, 0, canvasWidth - 1, canvasHeight - 1);
}

void Canvas::markCellsDirtyRegion(int x0, int y0, int x1, int y1)
{
	if (x0 > x1)
	{
		const int t = x0;
		x0 = x1;
		x1 = t;
	}
	if (y0 > y1)
	{
		const int t = y0;
		y0 = y1;
		y1 = t;
	}
	if (x0 < 0)
	{
		x0 = 0;
	}
	if (y0 < 0)
	{
		y0 = 0;
	}
	if (x1 >= canvasWidth)
	{
		x1 = canvasWidth - 1;
	}
	if (y1 >= canvasHeight)
	{
		y1 = canvasHeight - 1;
	}
	if (x0 > x1 || y0 > y1)
	{
		return;
	}
	cellsDirty = true;
	cellsDirtyRect.includeRect(x0, y0, x1, y1);
	noteUploadRegion(x0, y0, x1, y1);
}

void Canvas::setFadeSpeed(float speed)
{
	// R8 palette path snaps colors; keep value for console/env compatibility.
	if (speed < 0.0f)
	{
		speed = 0.0f;
	}
	fadeSpeed = speed;
}

void Canvas::onTargetsRebuilt()
{
	// Life→GPU upload is already flagged via markCellsDirty*; just clear the logical flag.
	cellsDirty = false;
	cellsDirtyRect.clear();
}

void Canvas::tickVisual(float dt)
{
	// No dual float RGB fade on the R8 path — colors come from the palette.
	(void)dt;
}

void Canvas::DrawImpl()
{
}

bool Canvas::AppendCommands(Renderer* r)
{
	ZoneScopedN("Canvas.AppendCommands");
	if (!isVisible() || !gpuReady || !r)
	{
		if (!isVisible())
		{
			return true;
		}
		return false;
	}

	// R8 cell texture: dirty-rect upload (PBO path inside GLTexture).
	if (textureUploadPending && lifeCanvas)
	{
		ZoneScopedN("Canvas.UpdateCellTexture");
		int x = 0;
		int y = 0;
		int w = canvasWidth;
		int h = canvasHeight;
		const void* data = lifeCanvas;
		int rowStride = 0;

		if (uploadDirtyRect.valid())
		{
			x = uploadDirtyRect.minX;
			y = uploadDirtyRect.minY;
			w = uploadDirtyRect.width();
			h = uploadDirtyRect.height();
			if (x < 0)
			{
				x = 0;
			}
			if (y < 0)
			{
				y = 0;
			}
			if (x + w > canvasWidth)
			{
				w = canvasWidth - x;
			}
			if (y + h > canvasHeight)
			{
				h = canvasHeight - y;
			}
			if (w > 0 && h > 0)
			{
				const size_t offset =
					static_cast<size_t>(y) * static_cast<size_t>(canvasWidth) + static_cast<size_t>(x);
				data = lifeCanvas + offset;
				rowStride = canvasWidth;
			}
			else
			{
				x = 0;
				y = 0;
				w = canvasWidth;
				h = canvasHeight;
				data = lifeCanvas;
				rowStride = 0;
			}
		}

		r->pushUpdateTexture(
			cellTextureHandle,
			x,
			y,
			w,
			h,
			1,
			data,
			rowStride);
		textureUploadPending = false;
		uploadDirtyRect.clear();
	}

	// Palette: rare full 256×1 RGB upload on ruleset change.
	if (paletteUploadPending)
	{
		ZoneScopedN("Canvas.UpdatePalette");
		r->pushUpdateTexture(
			paletteTextureHandle,
			0,
			0,
			kPaletteSize,
			1,
			3,
			paletteRgb,
			0);
		paletteUploadPending = false;
	}

	PipelineState ps;
	ps.depthTestEnabled = false;
	ps.blendEnabled = false;
	ps.faceCullingEnabled = false;
	ps.primitives = Primitives::Triangles;
	r->pushPipelineState(ps);

	r->pushSetShader(shaderHandle);
	r->pushSetMesh(meshHandle);
	r->pushSetTexture(cellTextureHandle, 0);
	r->pushSetTexture(paletteTextureHandle, 1);

	std::array<int, 2> dims = window->getWindowDimensions();
	float aspect = static_cast<float>(dims[0]) / static_cast<float>(dims[1]);
	glm::mat4 mvp = camera->GetMVPMatrix(aspect);
	r->pushUniformMat4("uMVP", &mvp[0][0]);
	r->pushUniformInt("uCellTexture", 0);
	r->pushUniformInt("uPalette", 1);
	r->pushDrawIndexed(6, 0);

	return true;
}
