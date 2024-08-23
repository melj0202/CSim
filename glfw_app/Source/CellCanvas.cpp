#include "CellCanvas.h"

void CellCanvas::setCanvasPixel(const int x, const int y, const std::array<unsigned char, 3> &colorVal) { 
	lifeCanvas[canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x] = colorVal[0];
	lifeCanvas[(canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x) + 1] = colorVal[1];
	lifeCanvas[(canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x) + 2] = colorVal[2];
}

void CellCanvas::getCanvasPixel(const int x, const int y, std::array<unsigned char, 3> &value) {

	value[0] = lifeCanvas[canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x];
	value[1] = lifeCanvas[canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x + 1];
	value[2] = lifeCanvas[canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x + 2];
}