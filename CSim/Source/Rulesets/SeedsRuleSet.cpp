#include "SeedsRuleSet.h"
#include "../CellCanvas.h"

enum CellStates {
	CELL_DEAD = 1,
	CELL_ALIVE = 0
};

//Determines the state of the cell based on the current state of the cell and the amount of neighbors
void SeedsRuleSet::evaluateNeighbors(unsigned char& cell, const unsigned char& ne, const int& x, const int& y) const
{
	if (cell == CellStates::CELL_DEAD && ne == 2) {
		setCanvasPixel(x, y, CellStates::CELL_ALIVE);
	}
	else {
		setCanvasPixel(x, y, CellStates::CELL_DEAD);
	}
}

void SeedsRuleSet::evalCell(const unsigned char& target, unsigned char dest[3]) const {
	if (target == CellStates::CELL_DEAD) memset(dest, 255, 3);
	else memset(dest, 0, 3);
}