#include "CellCanvas.h"

CellCanvas::CellCanvas(const int , const int )
{
	lifeCanvas = std::vector<unsigned char>(canvasWidth * canvasHeight * 3, 0);
}

void CellCanvas::setCanvasPixel(const int x, const int y, const std::vector<unsigned char> colorVal) { 
	lifeCanvas[canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x] = colorVal[0];
	lifeCanvas[canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x + 1] = colorVal[1];
	lifeCanvas[canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x + 2] = colorVal[2];
}

std::vector<unsigned char> CellCanvas::getCanvasPixel(const int x, const int y) {
	return std::vector<unsigned char> {
		lifeCanvas[canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x],
		lifeCanvas[canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x + 1],
		lifeCanvas[canvasWidth * 3 * (canvasHeight - 1 - y) + 3 * x + 2]
	};
}