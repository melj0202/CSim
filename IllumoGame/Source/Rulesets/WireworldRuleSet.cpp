#include "WireworldRuleSet.h"
#include <cstring>

unsigned char
WireworldRuleSet::nextState(unsigned char cell,
                            unsigned char headNeighbors) const
{
  // Simultaneous update (Silverman / Dewdney):
  // empty → empty
  // head → tail
  // tail → conductor
  // conductor → head iff exactly 1 or 2 Moore neighbors are heads
  if (cell == CELL_EMPTY) {
    return CELL_EMPTY;
  }
  if (cell == CELL_HEAD) {
    return CELL_TAIL;
  }
  if (cell == CELL_TAIL) {
    return CELL_CONDUCTOR;
  }
  if (cell == CELL_CONDUCTOR) {
    if (headNeighbors == 1 || headNeighbors == 2) {
      return CELL_HEAD;
    }
    return CELL_CONDUCTOR;
  }
  // Unknown values treated as empty.
  return CELL_EMPTY;
}

void
WireworldRuleSet::evalCell(const unsigned char& target,
                           unsigned char dest[3]) const
{
  // High-contrast palette (empty light, copper gold, head blue, tail red).
  if (target == CELL_EMPTY) {
    std::memset(dest, 255, 3);
  } else if (target == CELL_HEAD) {
    dest[0] = 0;
    dest[1] = 80;
    dest[2] = 255;
  } else if (target == CELL_TAIL) {
    dest[0] = 255;
    dest[1] = 40;
    dest[2] = 40;
  } else if (target == CELL_CONDUCTOR) {
    dest[0] = 255;
    dest[1] = 200;
    dest[2] = 40;
  } else {
    std::memset(dest, 200, 3);
  }
}
