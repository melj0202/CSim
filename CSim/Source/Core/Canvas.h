#pragma once
#include "Init/MacroDefs.h"
#include <vector>
#include <array>
#include <cstring>
#include "Rendering/Drawable.h"
#include "System/PoolAlloc.h"
#include "System/ModuleObject.h"


class Camera;
class IRenderWindow;
class RenderWindow;

struct Chunk {

};

struct Canvas {
	

	public:
	Canvas(int width, int height, IRenderWindow* window, Camera* camera) : ModuleObject() {
		this->window = window;
		this->camera = camera;
		initCanvas(width, height);
	};
	~Canvas() { freeCanvas(); };
	__CSIM_FORCE_INLINE__ std::array<int, 2> getDimensions() { return std::array<int, 2> {canvasWidth, canvasHeight}; };
	__CSIM_FORCE_INLINE__ void clearCanvas() { memset(lifeCanvas, 1, canvasWidth * canvasHeight); };
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
		texCanvasBuffer = nullptr;
		lifeCanvas = nullptr;
		if (canvasID) {
			glDeleteTextures(1, canvasID);
			delete canvasID;
			canvasID = nullptr;
		}
	};;
	void DrawImpl();

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
		//PoolAlloc<Chunk>* ChunkPool;
};
