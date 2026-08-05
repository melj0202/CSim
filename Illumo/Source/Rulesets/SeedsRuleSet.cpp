#include "SeedsRuleSet.h"
#include <cstring>

namespace {
const unsigned char CELL_DEAD = 1;
const unsigned char CELL_ALIVE = 0;
}

unsigned char SeedsRuleSet::nextState(unsigned char cell, unsigned char aliveNeighbors) const
{
	// B2/S — birth on 2, no survival
	(void)cell;
	return (aliveNeighbors == 2) ? CELL_ALIVE : CELL_DEAD;
}

void SeedsRuleSet::evalCell(const unsigned char& target, unsigned char dest[3]) const
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
