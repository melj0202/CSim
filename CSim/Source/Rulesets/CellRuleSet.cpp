#include "CellRuleSet.h"


#define CELL_DEAD 0

void CellRuleSet::calcGeneration(const int &x_start, const int &y_start, const int &x_end, const int &y_end) const
{
	const int canvasVectorWidth = x_end - x_start;
	const int canvasVectorHeight = y_end - y_start;
	unsigned char temp;

	for (int j = 0; j < canvasVectorHeight; j += BLOCK_Y) {
		for (int i = 0; i < canvasVectorWidth; i += BLOCK_X) {
			for (int by = 0; by < BLOCK_Y; by++) {
				for (int bx = 0; bx < BLOCK_X; bx++) {
					ne[i + bx][j + by] = countNeighbors(i + bx, j + by, canvasVectorWidth, canvasVectorHeight);
				}
			}
		}
	}

	for (int i = 0; i < canvasVectorWidth; i += BLOCK_X) {
		for (int j = 0; j < canvasVectorHeight; j += BLOCK_Y) {
			for (int by = 0; by < BLOCK_Y; by++) {
				for (int bx = 0; bx < BLOCK_X; bx++) {
					temp = getCanvasPixel(i + bx, j + by);
					evaluateNeighbors(temp, ne[i + bx][j + by], i + bx, j + by);
				}
			}

		}
	}
}

int CellRuleSet::countNeighbors(const int &r, const int &c, const int &w, const int &h) const
{
	int i2 = 0;
	int j2 = 0;
	int count = 0;
	for (int i = r - 1; i <= r + 1; i++) {
		for (int j = c - 1; j <= c + 1; j++) {
			i2 = i;
			j2 = j;
			if (i == r && j == c) continue;
			if (i < 0) i2 = w - 1;
			if (j < 0) j2 = h - 1;
			if (i >= w) i2 = 0;
			if (j >= h) j2 = 0;
			if (!getCanvasPixel(i2, j2)) count++;
		}
	}
	return count;
}
