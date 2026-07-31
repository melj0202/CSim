#pragma once
#include "Foundation/MacroDefs.h"
#include <vector>
#include <array>
#include <cstring>
#include "Rendering/Drawable.h"
#include "Services/PoolAlloc.h"

class Camera;
class IRenderWindow;
class Renderer;
class RuleSet;

// Inclusive axis-aligned cell/texel region. Invalid when maxX < minX.
struct DirtyRect {
	int minX;
	int minY;
	int maxX;
	int maxY;

	void clear()
	{
		minX = 0;
		minY = 0;
		maxX = -1;
		maxY = -1;
	}

	bool valid() const { return maxX >= minX && maxY >= minY; }

	void setFull(int width, int height)
	{
		if (width <= 0 || height <= 0)
		{
			clear();
			return;
		}
		minX = 0;
		minY = 0;
		maxX = width - 1;
		maxY = height - 1;
	}

	void include(int x, int y)
	{
		if (!valid())
		{
			minX = maxX = x;
			minY = maxY = y;
			return;
		}
		if (x < minX)
		{
			minX = x;
		}
		if (y < minY)
		{
			minY = y;
		}
		if (x > maxX)
		{
			maxX = x;
		}
		if (y > maxY)
		{
			maxY = y;
		}
	}

	void includeRect(int x0, int y0, int x1, int y1)
	{
		include(x0, y0);
		include(x1, y1);
	}

	int width() const { return valid() ? (maxX - minX + 1) : 0; }
	int height() const { return valid() ? (maxY - minY + 1) : 0; }
};

// Domain + view + GPU enroll in one type (D-C1 — intentional monolith for current scale).
// Domain: lifeCanvas (1 byte/cell).
// View: CPU palette → targetRgb; displayRgb eases toward targets; RGB texture upload.
// GPU: enrolled mesh/shader/texture handles; AppendCommands (pure-token).
// Split into LifeGrid + CanvasView only if headless pure-sim or large grids demand it.
struct Canvas : public Drawable<Canvas> {

public:
	static const int kPaletteSize = 256;

	Canvas(int width, int height, IRenderWindow* window, Camera* camera, Renderer* renderer);
	~Canvas();

	__CSIM_FORCE_INLINE__ std::array<int, 2> getDimensions()
	{
		return std::array<int, 2>{canvasWidth, canvasHeight};
	}

	__CSIM_FORCE_INLINE__ void clearCanvas()
	{
		memset(lifeCanvas, 1, static_cast<size_t>(canvasWidth * canvasHeight));
		const int n = canvasWidth * canvasHeight * 3;
		for (int i = 0; i < n; ++i)
		{
			displayRgb[i] = 1.0f;
			targetRgb[i] = 1.0f;
			texCanvasBuffer[i] = 255;
		}
		cellsDirty = false;
		cellsDirtyRect.clear();
		fadeActive = false;
		fadeDirtyRect.clear();
		textureUploadPending = true;
		uploadDirtyRect.setFull(canvasWidth, canvasHeight);
	}

	__CSIM_FORCE_INLINE__ bool inBounds(const int& x, const int& y) const
	{
		return x >= 0 && y >= 0 && x < canvasWidth && y < canvasHeight && lifeCanvas != nullptr;
	}

	// Logical cell write. Marks life dirty for visual target rebuild (not GPU yet).
	__CSIM_FORCE_INLINE__ bool setCanvasPixel(const int& x, const int& y, const unsigned char& colorVal)
	{
		if (!inBounds(x, y))
		{
			return false;
		}
		const int idx = canvasWidth * y + x;
		if (lifeCanvas[idx] != colorVal)
		{
			lifeCanvas[idx] = colorVal;
			cellsDirty = true;
			cellsDirtyRect.include(x, y);
		}
		return true;
	}

	__CSIM_FORCE_INLINE__ unsigned char getCanvasPixel(const int& x, const int& y) const
	{
		if (!inBounds(x, y))
		{
			return 1;
		}
		return lifeCanvas[canvasWidth * y + x];
	}

	void initCanvas(const int& width, const int& height);
	void freeCanvas();

	void DrawImpl();
	bool AppendCommands(Renderer* renderer) override;

	// Map cell state → target display color via palette (used by updateVisualTargets).
	void setTargetColor(int cellIndex, unsigned char r, unsigned char g, unsigned char b);
	// Apply palette[life] as targets for dirty life region (or full grid).
	void rebuildTargetsFromLife();
	void rebuildPalette(const RuleSet* rules);
	void rebuildDefaultPalette();

	void setFadeSpeed(float speed);
	float getFadeSpeed() const { return fadeSpeed; }
	void tickVisual(float dt);
	void snapVisualToTargets();

	bool isCellsDirty() const { return cellsDirty; }
	void markCellsDirty();
	void markCellsDirtyRegion(int x0, int y0, int x1, int y1);
	bool hasCellsDirtyRegion() const { return cellsDirty && cellsDirtyRect.valid(); }
	const DirtyRect& getCellsDirtyRegion() const { return cellsDirtyRect; }
	void onTargetsRebuilt();
	bool isFadeActive() const { return fadeActive; }
	bool isTextureUploadPending() const { return textureUploadPending; }
	const DirtyRect& getUploadDirtyRegion() const { return uploadDirtyRect; }
	const unsigned char* getPaletteRgb() const { return paletteRgb; }
	const unsigned char* getDisplayTexBuffer() const { return texCanvasBuffer; }

	int canvasWidth;
	int canvasHeight;
	unsigned char* lifeCanvas;
	unsigned char* texCanvasBuffer;
	IRenderWindow* window;
	Camera* camera;
	Renderer* renderer;

private:
	void enrollGpuResources();
	void noteTexelChanged(int cellX, int cellY);
	void writeTexelFromDisplay(int cellIndex, bool* anyByteChange);

	std::array<float, 32> vertices;
	std::array<unsigned int, 6> indices;

	unsigned char paletteRgb[kPaletteSize * 3];
	float* displayRgb;
	float* targetRgb;
	float fadeSpeed;

	unsigned long meshHandle;
	unsigned long shaderHandle;
	unsigned long displayTextureHandle;
	bool gpuReady;

	bool cellsDirty;
	bool fadeActive;
	bool textureUploadPending;

	DirtyRect cellsDirtyRect;
	DirtyRect fadeDirtyRect;
	DirtyRect uploadDirtyRect;
};
