#include "GameOfLifeRuleSet.h"
#include "CellCanvas.h"

enum CellStates {
		CELL_DEAD = 1,
		CELL_ALIVE = 0
	};

void GameOfLifeRuleSet::calcGeneration(const int x_start, const int y_start, const int x_end, const int y_end) const
{
	const int canvasVectorWidth = x_end - x_start;
	const int canvasVectorHeight = y_end - y_start;
	unsigned char temp;
	std::vector<std::vector<unsigned char>> ne(canvasVectorWidth, std::vector<unsigned char>(canvasVectorHeight));
	for (int i = 0; i < ne.size(); i++) {
		fill(ne[i].begin(), ne[i].end(), 0);
	}

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
					if (temp == CellStates::CELL_ALIVE && (ne[i + bx][j + by] == 2 || ne[i + bx][j + by] == 3)) {
						setCanvasPixel(i + bx, j + by, CellStates::CELL_ALIVE);
					}
					else if (!(temp == CellStates::CELL_ALIVE) && ne[i + bx][j + by] == 3) {
						setCanvasPixel(i + bx, j + by, CellStates::CELL_ALIVE);
					}
					else {
						setCanvasPixel(i + bx, j + by, CellStates::CELL_DEAD);
					}
				}
			}

		}
	}
}

void GameOfLifeRuleSet::evalCell(const unsigned char& target, unsigned char dest[3]) const {
	if (target == CellStates::CELL_DEAD) memset(dest, 255, 3);
	else memset(dest, 0, 3);
}