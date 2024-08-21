#pragma once
#include <vector>

class CellCanvas {
public:
	CellCanvas(const int width, const int height) : canvasWidth(width), canvasHeight(height) {};
	std::vector<int> getDimensions() const { return std::vector<int> {canvasWidth, canvasHeight}; };
	void clearCanvas() { fill(lifeCanvas.begin(), lifeCanvas.end(), 0); };
	void setCanvasPixel(const int x, const int y, const std::vector<unsigned char> colorVal);
	std::vector<unsigned char> getCanvasPixel(const int x, const int y);
private:
	int canvasWidth;
	int canvasHeight;
	std::vector<unsigned char> lifeCanvas;
};