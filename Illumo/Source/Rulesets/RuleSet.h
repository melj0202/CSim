#pragma once
#include "Canvas.h"
#include <string>
#include <vector>

constexpr auto MAX_RULETAG_SIZE = 128;

// Base cellular-automaton ruleset.
// Generation is double-buffered: neighbors are read from the current
// lifeCanvas, next states are written to an internal buffer, then copied back
// once.
class RuleSet
{
public:
  Canvas* canvas;

  RuleSet(Canvas* targetCanvas)
    : canvas(targetCanvas)
  {
  }

  virtual ~RuleSet() = default;

  // Advance one generation over [x_start,x_end) × [y_start,y_end).
  // Normal path uses the full canvas (0,0,width,height).
  void calcGeneration(const int& x_start,
                      const int& y_start,
                      const int& x_end,
                      const int& y_end) const;

  // Map logical cell value → RGB display color.
  virtual void evalCell(const unsigned char& target,
                        unsigned char dest[3]) const
  {
    (void)target;
    (void)dest;
  }

  virtual std::string getRuleTag() { return "BASE_CLASS"; }

protected:
  // Pure transition: old cell + Moore neighbor count of *alive* (value==0)
  // cells. Does not write the canvas. Override in each ruleset.
  virtual unsigned char nextState(unsigned char cell,
                                  unsigned char aliveNeighbors) const
  {
    (void)aliveNeighbors;
    return cell;
  }

  // Fast toroidal Moore count of cells with value 0 (project "alive" encoding).
  static int countAliveNeighbors(const unsigned char* grid,
                                 int w,
                                 int h,
                                 int x,
                                 int y);

private:
  // Scratch next generation (mutable so calcGeneration can stay const like
  // before).
  mutable std::vector<unsigned char> nextGen;
};
