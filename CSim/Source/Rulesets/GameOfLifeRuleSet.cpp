#include "GameOfLifeRuleSet.h"
#include <cstring>

namespace {
const unsigned char CELL_DEAD = 1;
const unsigned char CELL_ALIVE = 0;
}

unsigned char GameOfLifeRuleSet::nextState(unsigned char cell, unsigned char aliveNeighbors) const
{
	// B3/S23
	if (cell == CELL_ALIVE)
	{
		return (aliveNeighbors == 2 || aliveNeighbors == 3) ? CELL_ALIVE : CELL_DEAD;
	}
	return (aliveNeighbors == 3) ? CELL_ALIVE : CELL_DEAD;
}

void GameOfLifeRuleSet::evalCell(const unsigned char& target, unsigned char dest[3]) const
{
	if (target == CELL_DEAD)
	{
		std::memset(dest, 255, 3);
	}
	else
	{
		std::memset(dest, 0, 3);
	}
}
