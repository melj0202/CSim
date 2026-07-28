#pragma once
#include "Foundation/MacroDefs.h"
#include <vector>
#include <array>
#include <cstring>
#include "Rendering/Drawable.h"
#include "Services/PoolAlloc.h"
#include "Engine/ModuleObject.h"

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

// Domain grid + token-based view (R8 cell texture + RGB palette, P4).
// lifeCanvas is the GPU source (1 byte/cell). Colors come from a 256-entry palette.
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
		cellsDirty = false;
		cellsDirtyRect.clear();
		textureUploadPending = true;
		uploadDirtyRect.setFull(canvasWidth, canvasHeight);
	}

	__CSIM_FORCE_INLINE__ void setCanvasPixel(const int& x, const int& y, const unsigned char& colorVal)
	{
		const int idx = canvasWidth * y + x;
		if (lifeCanvas[idx] != colorVal)
		{
			lifeCanvas[idx] = colorVal;
			cellsDirty = true;
			cellsDirtyRect.include(x, y);
			textureUploadPending = true;
			uploadDirtyRect.include(x, y);
		}
	}

	__CSIM_FORCE_INLINE__ unsigned char getCanvasPixel(const int& x, const int& y)
	{
		return lifeCanvas[canvasWidth * y + x];
	}

	void initCanvas(const int& width, const int& height);
	void freeCanvas();

	void DrawImpl();
	bool AppendCommands(Renderer* renderer) override;

	// Rebuild 256-entry RGB palette from ruleset evalCell (ruleset change / init).
	void rebuildPalette(const RuleSet* rules);
	// Default black/white palette when no ruleset is available yet.
	void rebuildDefaultPalette();

	// Fade API kept for env/console compatibility; R8 path snaps (no dual float RGB).
	void setFadeSpeed(float speed);
	float getFadeSpeed() const { return fadeSpeed; }
	void tickVisual(float dt);

	// --- Dirty / skip API (performance) ---
	bool isCellsDirty() const { return cellsDirty; }
	void markCellsDirty();
	void markCellsDirtyRegion(int x0, int y0, int x1, int y1);
	bool hasCellsDirtyRegion() const { return cellsDirty && cellsDirtyRect.valid(); }
	const DirtyRect& getCellsDirtyRegion() const { return cellsDirtyRect; }
	// After life changes have been queued for GPU upload.
	void onTargetsRebuilt();
	bool isTextureUploadPending() const { return textureUploadPending; }
	bool isPaletteUploadPending() const { return paletteUploadPending; }
	const DirtyRect& getUploadDirtyRegion() const { return uploadDirtyRect; }
	const unsigned char* getPaletteRgb() const { return paletteRgb; }

	int canvasWidth;
	int canvasHeight;
	unsigned char* lifeCanvas;
	IRenderWindow* window;
	Camera* camera;
	Renderer* renderer;

private:
	void enrollGpuResources();
	void noteUploadRegion(int x0, int y0, int x1, int y1);

	std::array<float, 32> vertices;
	std::array<unsigned int, 6> indices;

	// 256×1 RGB palette (CPU staging for GPU texture).
	unsigned char paletteRgb[kPaletteSize * 3];
	float fadeSpeed;

	unsigned long meshHandle;
	unsigned long shaderHandle;
	unsigned long cellTextureHandle;
	unsigned long paletteTextureHandle;
	bool gpuReady;

	bool cellsDirty;
	bool textureUploadPending;
	bool paletteUploadPending;

	DirtyRect cellsDirtyRect;
	DirtyRect uploadDirtyRect;
};
