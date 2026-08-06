#include "LifeWithoutDeathRuleSet.h"
#include <cstring>

namespace {
const unsigned char CELL_DEAD = 1;
const unsigned char CELL_ALIVE = 0;
}

unsigned char
LifeWithoutDeathRuleSet::nextState(unsigned char cell,
                                   unsigned char aliveNeighbors) const
{
  // B3/S012345678 — once alive, stay alive; birth on 3
  if (cell == CELL_ALIVE) {
    return CELL_ALIVE;
  }
  return (aliveNeighbors == 3) ? CELL_ALIVE : CELL_DEAD;
}

void
LifeWithoutDeathRuleSet::evalCell(const unsigned char& target,
                                  unsigned char dest[3]) const
{
  if (target == CELL_DEAD) {
    std::memset(dest, 255, 3);
  } else {
    std::memset(dest, 0, 3);
  }
}
