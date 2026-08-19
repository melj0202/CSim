#include "DayAndNightRuleSet.h"
#include <cstring>

namespace {
const unsigned char CELL_DEAD = 1;
const unsigned char CELL_ALIVE = 0;
}

unsigned char
DayAndNightRuleSet::nextState(unsigned char cell,
                              unsigned char aliveNeighbors) const
{
  // B3678/S34678
  if (cell == CELL_DEAD) {
    return (aliveNeighbors == 3 || aliveNeighbors == 6 || aliveNeighbors == 7 ||
            aliveNeighbors == 8)
             ? CELL_ALIVE
             : CELL_DEAD;
  }
  // alive
  return (aliveNeighbors == 3 || aliveNeighbors == 4 || aliveNeighbors == 6 ||
          aliveNeighbors == 7 || aliveNeighbors == 8)
           ? CELL_ALIVE
           : CELL_DEAD;
}

void
DayAndNightRuleSet::evalCell(const unsigned char& target,
                             unsigned char dest[3]) const
{
  if (target == CELL_DEAD) {
    std::memset(dest, 255, 3);
  } else {
    std::memset(dest, 0, 3);
  }
}
