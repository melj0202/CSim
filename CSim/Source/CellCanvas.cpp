#include "CellCanvas.h"
#include <iostream>
#include <cstdint>

int CellCanvas::canvasWidth = 80;
int CellCanvas::canvasHeight = 60;
unsigned char* CellCanvas::lifeCanvas = new unsigned char[80 * 60];
unsigned char* CellCanvas::texCanvasBuffer = new unsigned char[80 * 60 * 3];

/*
	The getter and setter translate the 2D coordinates specified to a 1D coordinate...
*/
__CSIM_FORCE_INLINE__ void setCanvasPixel(const int &x, const int &y, const unsigned char &colorVal) {
	CellCanvas::lifeCanvas[CellCanvas::canvasWidth * (CellCanvas::canvasHeight - 1 - y) + x] = colorVal;
};

__CSIM_FORCE_INLINE__ unsigned char getCanvasPixel(const int &x, const int &y) {
	return CellCanvas::lifeCanvas[CellCanvas::canvasWidth * (CellCanvas::canvasHeight - 1 - y) + x];
};

//Init a new cell canvas 
void initCanvas(const int &width, const int &height) { 

	//Init the canvas
	CellCanvas::canvasWidth = width;
	CellCanvas::canvasHeight = height;
	CellCanvas::lifeCanvas = new unsigned char[width*height];
	memset(CellCanvas::lifeCanvas, 1, width * height);

	//Init the texture representation of the canvas
	CellCanvas::texCanvasBuffer = new unsigned char[width * height * 3];
	memset(CellCanvas::texCanvasBuffer, 255, width * height*3);
};