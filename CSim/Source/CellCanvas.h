#pragma once
#include <vector>
#include <array>

struct CellCanvas {
	
	static int canvasWidth;
	static int canvasHeight;
	static unsigned char* lifeCanvas;
	static unsigned char* texCanvasBuffer;
};

static inline std::array<int, 2> getDimensions() { return std::array<int, 2> {CellCanvas::canvasWidth, CellCanvas::canvasHeight}; };
static inline void clearCanvas() { memset(CellCanvas::lifeCanvas, 1, CellCanvas::canvasWidth * CellCanvas::canvasHeight); };
extern void setCanvasPixel(const int x, const int y, const unsigned char &colorVal);
extern unsigned char getCanvasPixel(const int x, const int y);
extern void initCanvas(const int width, const int height);
static inline void freeCanvas() { 
	delete[] CellCanvas::texCanvasBuffer;
	delete[] CellCanvas::lifeCanvas; 
};