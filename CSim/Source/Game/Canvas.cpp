#include "Canvas.h"
#include "IRenderWindow.h"
#include "Logger.h"
#include "Rendering/Camera.h"
#include "Rendering/Renderer.h"
#include "Rendering/IShaderProgram.h"
#include "Rulesets/RuleSet.h"
#include <array>
#include <cmath>
#include <cstring>
#include <glm/fwd.hpp>
#include <tracy/Tracy.hpp>

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
	displayTextureHandle = 0;
	gpuReady = false;
	canvasWidth = 0;
	canvasHeight = 0;
	cellsDirty = true;
	fadeActive = false;
	textureUploadPending = true;
	cellsDirtyRect.clear();
	fadeDirtyRect.clear();
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
			paletteRgb[base + 0] = 0;
			paletteRgb[base + 1] = 164;
			paletteRgb[base + 2] = 128;
		}
	}
}

void Canvas::rebuildPalette(const RuleSet* rules)
{
	ZoneScopedN("Canvas.rebuildPalette");
	if (!rules)
	{
		rebuildDefaultPalette();
	}
	else
	{
		for (int s = 0; s < kPaletteSize; ++s)
		{
			unsigned char rgb[3] = {255, 255, 255};
			rules->evalCell(static_cast<unsigned char>(s), rgb);
			const int base = s * 3;
			paletteRgb[base + 0] = rgb[0];
			paletteRgb[base + 1] = rgb[1];
			paletteRgb[base + 2] = rgb[2];
		}
	}
	// New colors for existing life values — rebuild all display targets.
	markCellsDirty();
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
	fadeActive = false;
	fadeDirtyRect.clear();
	textureUploadPending = true;
	uploadDirtyRect.setFull(width, height);

	enrollGpuResources();
	Logger::LogTrace("Canvas initialized (fade + RGB display texture)");
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

	// RGB display texture (faded colors). Domain remains lifeCanvas on CPU.
	displayTextureHandle = renderer->allocateHandle();
	renderer->enrollTexture(
		texCanvasBuffer,
		canvasWidth,
		canvasHeight,
		3,
		displayTextureHandle);

	gpuReady = true;
	textureUploadPending = true;
	uploadDirtyRect.setFull(canvasWidth, canvasHeight);
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
	gpuReady = false;
}

void Canvas::markCellsDirty()
{
	cellsDirty = true;
	cellsDirtyRect.setFull(canvasWidth, canvasHeight);
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
}

void Canvas::setFadeSpeed(float speed)
{
	if (speed < 0.0f)
	{
		speed = 0.0f;
	}
	if (speed != fadeSpeed)
	{
		fadeSpeed = speed;
		if (fadeActive || cellsDirty)
		{
			fadeActive = true;
			if (!fadeDirtyRect.valid())
			{
				fadeDirtyRect.setFull(canvasWidth, canvasHeight);
			}
		}
	}
}

void Canvas::setTargetColor(int cellIndex, unsigned char r, unsigned char g, unsigned char b)
{
	if (cellIndex < 0 || cellIndex >= canvasWidth * canvasHeight)
	{
		return;
	}
	const int base = cellIndex * 3;
	const float rf = static_cast<float>(r) / 255.0f;
	const float gf = static_cast<float>(g) / 255.0f;
	const float bf = static_cast<float>(b) / 255.0f;
	if (targetRgb[base + 0] != rf || targetRgb[base + 1] != gf || targetRgb[base + 2] != bf)
	{
		targetRgb[base + 0] = rf;
		targetRgb[base + 1] = gf;
		targetRgb[base + 2] = bf;
		fadeActive = true;
		const int cellX = cellIndex % canvasWidth;
		const int cellY = cellIndex / canvasWidth;
		fadeDirtyRect.include(cellX, cellY);
	}
}

void Canvas::rebuildTargetsFromLife()
{
	ZoneScopedN("Canvas.rebuildTargetsFromLife");
	if (!cellsDirty || !lifeCanvas)
	{
		return;
	}

	const int w = canvasWidth;
	if (cellsDirtyRect.valid())
	{
		for (int y = cellsDirtyRect.minY; y <= cellsDirtyRect.maxY; ++y)
		{
			for (int x = cellsDirtyRect.minX; x <= cellsDirtyRect.maxX; ++x)
			{
				const int i = y * w + x;
				const unsigned char state = lifeCanvas[i];
				const int p = static_cast<int>(state) * 3;
				setTargetColor(i, paletteRgb[p + 0], paletteRgb[p + 1], paletteRgb[p + 2]);
			}
		}
	}
	else
	{
		const int count = w * canvasHeight;
		for (int i = 0; i < count; ++i)
		{
			const unsigned char state = lifeCanvas[i];
			const int p = static_cast<int>(state) * 3;
			setTargetColor(i, paletteRgb[p + 0], paletteRgb[p + 1], paletteRgb[p + 2]);
		}
	}
	onTargetsRebuilt();
}

void Canvas::onTargetsRebuilt()
{
	cellsDirty = false;
	cellsDirtyRect.clear();
	if (fadeSpeed <= 0.0f)
	{
		snapVisualToTargets();
		fadeActive = false;
	}
}

void Canvas::noteTexelChanged(int cellX, int cellY)
{
	textureUploadPending = true;
	uploadDirtyRect.include(cellX, cellY);
}

void Canvas::writeTexelFromDisplay(int cellIndex, bool* anyByteChange)
{
	const int base = cellIndex * 3;
	const int cellX = cellIndex % canvasWidth;
	const int cellY = cellIndex / canvasWidth;
	bool changed = false;
	for (int c = 0; c < 3; ++c)
	{
		const float v = displayRgb[base + c] * 255.0f + 0.5f;
		const unsigned char b = static_cast<unsigned char>(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v));
		if (texCanvasBuffer[base + c] != b)
		{
			texCanvasBuffer[base + c] = b;
			changed = true;
		}
	}
	if (changed)
	{
		noteTexelChanged(cellX, cellY);
		if (anyByteChange)
		{
			*anyByteChange = true;
		}
	}
}

void Canvas::snapVisualToTargets()
{
	ZoneScopedN("Canvas.snapVisualToTargets");
	bool anyByteChange = false;

	if (fadeDirtyRect.valid())
	{
		for (int y = fadeDirtyRect.minY; y <= fadeDirtyRect.maxY; ++y)
		{
			for (int x = fadeDirtyRect.minX; x <= fadeDirtyRect.maxX; ++x)
			{
				const int cellIndex = y * canvasWidth + x;
				const int base = cellIndex * 3;
				displayRgb[base + 0] = targetRgb[base + 0];
				displayRgb[base + 1] = targetRgb[base + 1];
				displayRgb[base + 2] = targetRgb[base + 2];
				writeTexelFromDisplay(cellIndex, &anyByteChange);
			}
		}
	}
	else
	{
		const int count = canvasWidth * canvasHeight;
		for (int i = 0; i < count; ++i)
		{
			const int base = i * 3;
			displayRgb[base + 0] = targetRgb[base + 0];
			displayRgb[base + 1] = targetRgb[base + 1];
			displayRgb[base + 2] = targetRgb[base + 2];
			writeTexelFromDisplay(i, &anyByteChange);
		}
	}

	(void)anyByteChange;
	fadeActive = false;
	fadeDirtyRect.clear();
}

void Canvas::tickVisual(float dt)
{
	ZoneScopedN("Canvas.tickVisual");
	if (!fadeActive)
	{
		return;
	}

	if (dt < 0.0f)
	{
		dt = 0.0f;
	}
	if (fadeSpeed <= 0.0f)
	{
		snapVisualToTargets();
		return;
	}

	float alpha = 1.0f - expf(-fadeSpeed * dt);
	if (alpha > 1.0f)
	{
		alpha = 1.0f;
	}

	int x0 = 0;
	int y0 = 0;
	int x1 = canvasWidth - 1;
	int y1 = canvasHeight - 1;
	if (fadeDirtyRect.valid())
	{
		x0 = fadeDirtyRect.minX;
		y0 = fadeDirtyRect.minY;
		x1 = fadeDirtyRect.maxX;
		y1 = fadeDirtyRect.maxY;
	}

	bool stillFading = false;
	bool anyByteChange = false;
	const float eps = 0.002f;

	for (int y = y0; y <= y1; ++y)
	{
		for (int x = x0; x <= x1; ++x)
		{
			const int cellIndex = y * canvasWidth + x;
			const int base = cellIndex * 3;
			bool cellStill = false;
			for (int c = 0; c < 3; ++c)
			{
				const float target = targetRgb[base + c];
				float d = displayRgb[base + c];
				const float diff = target - d;
				if (diff > eps || diff < -eps)
				{
					d = d + diff * alpha;
					displayRgb[base + c] = d;
					const float diff2 = target - d;
					if (diff2 > eps || diff2 < -eps)
					{
						cellStill = true;
					}
					else
					{
						displayRgb[base + c] = target;
					}
				}
				else
				{
					displayRgb[base + c] = target;
				}
			}
			if (cellStill)
			{
				stillFading = true;
			}
			writeTexelFromDisplay(cellIndex, &anyByteChange);
		}
	}

	(void)anyByteChange;
	fadeActive = stillFading;
	if (!stillFading)
	{
		fadeDirtyRect.clear();
	}
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

	// RGB display texture: dirty-rect upload (PBO path inside GLTexture).
	if (textureUploadPending && texCanvasBuffer)
	{
		ZoneScopedN("Canvas.UpdateDisplayTexture");
		int x = 0;
		int y = 0;
		int w = canvasWidth;
		int h = canvasHeight;
		const void* data = texCanvasBuffer;
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
					(static_cast<size_t>(y) * static_cast<size_t>(canvasWidth) + static_cast<size_t>(x)) * 3u;
				data = texCanvasBuffer + offset;
				rowStride = canvasWidth;
			}
			else
			{
				x = 0;
				y = 0;
				w = canvasWidth;
				h = canvasHeight;
				data = texCanvasBuffer;
				rowStride = 0;
			}
		}

		r->pushUpdateTexture(
			displayTextureHandle,
			x,
			y,
			w,
			h,
			3,
			data,
			rowStride);
		textureUploadPending = false;
		uploadDirtyRect.clear();
	}

	PipelineState ps;
	ps.depthTestEnabled = false;
	ps.blendEnabled = false;
	ps.faceCullingEnabled = false;
	ps.primitives = Primitives::Triangles;
	r->pushPipelineState(ps);

	r->pushSetShader(shaderHandle);
	r->pushSetMesh(meshHandle);
	r->pushSetTexture(displayTextureHandle, 0);

	std::array<int, 2> dims = window->getWindowDimensions();
	float aspect = static_cast<float>(dims[0]) / static_cast<float>(dims[1]);
	glm::mat4 mvp = camera->GetMVPMatrix(aspect);
	r->pushUniformMat4("uMVP", &mvp[0][0]);
	r->pushUniformInt("uDisplayTexture", 0);
	r->pushDrawIndexed(6, 0);

	return true;
}
