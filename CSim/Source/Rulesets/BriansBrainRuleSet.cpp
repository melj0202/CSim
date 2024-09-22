#include "BrainsBrainRuleSet.h"
#include "../CellCanvas.h"

enum CellStates {
	CELL_DEAD = 1,
	CELL_DYING = 2,
	CELL_ALIVE = 0
};

void BrainsBrainRuleSet::evaluateNeighbors(unsigned char& cell, const unsigned char& ne, const int& x, const int& y) const
{
	if (cell == CellStates::CELL_DEAD && ne == 2) {
		setCanvasPixel(x, y, CellStates::CELL_ALIVE);
	}
	else if (cell == CellStates::CELL_ALIVE) {
		setCanvasPixel(x, y, CellStates::CELL_DYING);
	}
	else if (cell == CellStates::CELL_DYING) {
		setCanvasPixel(x, y, CellStates::CELL_DEAD);
	}
}

void BrainsBrainRuleSet::evalCell(const unsigned char& target, unsigned char dest[3]) const {
	if (target == CellStates::CELL_DEAD) memset(dest, 255, 3);
	else if (target == CellStates::CELL_ALIVE) memset(dest, 0, 3);
	else {
		dest[0] = 0;
		dest[2] = 128;
		dest[1] = 164;
	}
}