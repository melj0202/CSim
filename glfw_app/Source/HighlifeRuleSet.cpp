#include "HighlifeRuleSet.h"

struct CellStates {
	const std::array<unsigned char, 3> CELL_DEAD = { 255, 255, 255 };
	const std::array<unsigned char, 3> CELL_ALIVE = { 0, 0, 0 };
};

const CellStates lifeStates;

void HighlifeRuleSet::calcGeneration(const int x_start, const int y_start, const int x_end, const int y_end, CellCanvas& canvas) const
{
	const int canvasVectorWidth = x_end - x_start;
	const int canvasVectorHeight = y_end - y_start;
	std::array<unsigned char, 3> temp;
	std::vector<std::vector<int>> ne(canvasVectorWidth, std::vector<int>(canvasVectorHeight));
	for (int i = 0; i < ne.size(); i++) {
		fill(ne[i].begin(), ne[i].end(), 0);
	}

	for (int j = 0; j < canvasVectorHeight; j += BLOCK_Y) {
		for (int i = 0; i < canvasVectorWidth; i += BLOCK_X) {
			for (int by = 0; by < BLOCK_Y; by++) {
				for (int bx = 0; bx < BLOCK_X; bx++) {
					ne[i + bx][j + by] = countNeighbors(i + bx, j + by, canvasVectorWidth, canvasVectorHeight, canvas);
				}
			}
		}
	}

	for (int i = 0; i < canvasVectorWidth; i += BLOCK_X) {
		for (int j = 0; j < canvasVectorHeight; j += BLOCK_Y) {
			for (int by = 0; by < BLOCK_Y; by++) {
				for (int bx = 0; bx < BLOCK_X; bx++) {
					canvas.getCanvasPixel(i + bx, j + by, temp);
					if (temp == lifeStates.CELL_ALIVE && (ne[i + bx][j + by] == 2 || ne[i + bx][j + by] == 3)) {
						canvas.setCanvasPixel(i + bx, j + by, lifeStates.CELL_ALIVE);
					}
					else if (!(temp == lifeStates.CELL_ALIVE) && (ne[i + bx][j + by] == 3 || ne[i + bx][j + by] == 6)) {
						canvas.setCanvasPixel(i + bx, j + by, lifeStates.CELL_ALIVE);
					}
					else {
						canvas.setCanvasPixel(i + bx, j + by, lifeStates.CELL_DEAD);
					}
				}
			}

		}
	}
}
