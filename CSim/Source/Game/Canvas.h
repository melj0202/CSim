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
class RenderWindow;

struct Chunk {

};

struct Canvas : public Drawable<Canvas> {
	

	public:
	Canvas(int width, int height, IRenderWindow* window, Camera* camera) {
		this->window = window;
		this->camera = camera;
		initCanvas(width, height);
	};
	~Canvas() { freeCanvas(); };
	__CSIM_FORCE_INLINE__ std::array<int, 2> getDimensions() { return std::array<int, 2> {canvasWidth, canvasHeight}; };
	__CSIM_FORCE_INLINE__ void clearCanvas() {
		memset(lifeCanvas, 1, canvasWidth * canvasHeight);
		// Snap visual to cleared (dead/white) state so clear feels responsive
		const int n = canvasWidth * canvasHeight * 3;
		for (int i = 0; i < n; ++i)
		{
			displayRgb[i] = 1.0f;
			targetRgb[i] = 1.0f;
			texCanvasBuffer[i] = 255;
		}
	};
	__CSIM_FORCE_INLINE__ void setCanvasPixel(const int &x, const int &y, const unsigned char &colorVal) {
		lifeCanvas[canvasWidth * y + x] = colorVal;
	};
	__CSIM_FORCE_INLINE__ unsigned char getCanvasPixel(const int &x, const int &y) {
		return lifeCanvas[canvasWidth * y + x];
	};
	void initCanvas(const int &width, const int &height);
	void freeCanvas() {
		delete[] texCanvasBuffer;
		delete[] lifeCanvas;
		delete[] displayRgb;
		delete[] targetRgb;
		texCanvasBuffer = nullptr;
		lifeCanvas = nullptr;
		displayRgb = nullptr;
		targetRgb = nullptr;
		if (canvasID) {
			glDeleteTextures(1, canvasID);
			delete canvasID;
			canvasID = nullptr;
		}
	};;
	void DrawImpl();

	// Visual fade: targets are logical colors; display lerps toward them each frame.
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

	private:
		std::array<float, 32> vertices;
		std::array<unsigned char, 32> indices;
		unsigned int* canvasID;
		float* displayRgb;
		float* targetRgb;
		float fadeSpeed;
		//PoolAlloc<Chunk>* ChunkPool;
};
