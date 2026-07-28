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

struct Chunk {
};

// Domain grid + token-based view (Phase 3).
// CPU owns lifeCanvas / fade buffers; GPU resources are backend handles.
struct Canvas : public Drawable<Canvas> {

public:
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
	}

	__CSIM_FORCE_INLINE__ void setCanvasPixel(const int& x, const int& y, const unsigned char& colorVal)
	{
		lifeCanvas[canvasWidth * y + x] = colorVal;
	}

	__CSIM_FORCE_INLINE__ unsigned char getCanvasPixel(const int& x, const int& y)
	{
		return lifeCanvas[canvasWidth * y + x];
	}

	void initCanvas(const int& width, const int& height);
	void freeCanvas();

	// Legacy immediate path unused once AppendCommands is active.
	void DrawImpl();

	// Token path: update texture + draw textured quad.
	bool AppendCommands(Renderer* renderer) override;

	void setTargetColor(int cellIndex, unsigned char r, unsigned char g, unsigned char b);
	void setFadeSpeed(float speed) { fadeSpeed = speed; }
	float getFadeSpeed() const { return fadeSpeed; }
	void tickVisual(float dt);
	void snapVisualToTargets();

	int canvasWidth;
	int canvasHeight;
	unsigned char* lifeCanvas;
	unsigned char* texCanvasBuffer;
	IRenderWindow* window;
	Camera* camera;
	Renderer* renderer;

private:
	void enrollGpuResources();

	std::array<float, 32> vertices;
	std::array<unsigned int, 6> indices;
	float* displayRgb;
	float* targetRgb;
	float fadeSpeed;

	// Backend registry handles (opaque table IDs)
	unsigned long meshHandle;
	unsigned long shaderHandle;
	unsigned long textureHandle;
	bool gpuReady;
};
