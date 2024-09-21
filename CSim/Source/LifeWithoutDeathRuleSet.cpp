#include "LifeWithoutDeathRuleSet.h"
#include "CellCanvas.h"

enum CellStates {
	CELL_DEAD = 1,
	CELL_ALIVE = 0
};

void LifeWithoutDeathRuleSet::evaluateNeighbors(unsigned char& cell, const unsigned char& ne, const int& x, const int& y) const
{
	if (cell == CellStates::CELL_DEAD && ne == 3) {
		setCanvasPixel(x, y, CellStates::CELL_ALIVE);
	}
	else if (cell == CellStates::CELL_ALIVE && ne >= 0) {
		setCanvasPixel(x, y, CellStates::CELL_ALIVE);
	}
	else {
		setCanvasPixel(x, y, CellStates::CELL_DEAD);
	}
}

void LifeWithoutDeathRuleSet::evalCell(const unsigned char& target, unsigned char dest[3]) const {
	if (target == CellStates::CELL_DEAD) memset(dest, 255, 3);
	else memset(dest, 0, 3);
}