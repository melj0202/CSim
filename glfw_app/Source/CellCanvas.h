#pragma once
#include <vector>
#include <array>

class CellCanvas {
public:
	CellCanvas(const int width, const int height) : canvasWidth(width), canvasHeight(height) { lifeCanvas = std::vector<unsigned char>(canvasWidth * canvasHeight * 3, 255); };
	std::vector<int> getDimensions() const { return std::vector<int> {canvasWidth, canvasHeight}; };
	void clearCanvas() { fill(lifeCanvas.begin(), lifeCanvas.end(), 0); };
	void setCanvasPixel(const int x, const int y, const std::array<unsigned char, 3> &colorVal);
	void getCanvasPixel(const int x, const int y, std::array<unsigned char, 3> &value);
	std::vector<unsigned char> getCanvasState() const { return lifeCanvas; };
private:
	int canvasWidth;
	int canvasHeight;
	std::vector<unsigned char> lifeCanvas;
};