#include "RuleSet.h"
#include <cstring>
#include <tracy/Tracy.hpp>

// Project convention: 0 = "alive" for neighbor counting (see historical
// !getCanvasPixel).
int
RuleSet::countAliveNeighbors(const unsigned char* grid,
                             int w,
                             int h,
                             int x,
                             int y)
{
  const int xm = (x > 0) ? (x - 1) : (w - 1);
  const int xp = (x + 1 < w) ? (x + 1) : 0;
  const int ym = (y > 0) ? (y - 1) : (h - 1);
  const int yp = (y + 1 < h) ? (y + 1) : 0;

  const unsigned char* rowm =
    grid + static_cast<size_t>(ym) * static_cast<size_t>(w);
  const unsigned char* row =
    grid + static_cast<size_t>(y) * static_cast<size_t>(w);
  const unsigned char* rowp =
    grid + static_cast<size_t>(yp) * static_cast<size_t>(w);

  // Branchless-ish: compare to 0 and sum (bool→int).
  int count = 0;
  count += (rowm[xm] == 0);
  count += (rowm[x] == 0);
  count += (rowm[xp] == 0);
  count += (row[xm] == 0);
  count += (row[xp] == 0);
  count += (rowp[xm] == 0);
  count += (rowp[x] == 0);
  count += (rowp[xp] == 0);
  return count;
}

void
RuleSet::calcGeneration(const int& x_start,
                        const int& y_start,
                        const int& x_end,
                        const int& y_end) const
{
  ZoneScopedN("Rule.calcGeneration");
  if (!canvas || !canvas->lifeCanvas) {
    return;
  }

  // Generation is always computed for the full lifeCanvas (toroidal wrap).
  // Callers pass full extents from CellGameModule; sub-rects are ignored.
  (void)x_start;
  (void)y_start;
  (void)x_end;
  (void)y_end;

  const int width = canvas->canvasWidth;
  const int height = canvas->canvasHeight;
  if (width <= 0 || height <= 0) {
    return;
  }

  const size_t total = static_cast<size_t>(width) * static_cast<size_t>(height);
  const unsigned char* src = canvas->lifeCanvas;

  if (nextGen.size() != total) {
    nextGen.resize(total);
  }

  {
    ZoneScopedN("Rule.evalPass");
    // y-major scan: contiguous rows for better cache locality.
    for (int y = 0; y < height; ++y) {
      const size_t rowBase =
        static_cast<size_t>(y) * static_cast<size_t>(width);
      for (int x = 0; x < width; ++x) {
        const size_t i = rowBase + static_cast<size_t>(x);
        const unsigned char n = static_cast<unsigned char>(
          countAliveNeighbors(src, width, height, x, y));
        nextGen[i] = nextState(src[i], n);
      }
    }
  }

  {
    ZoneScopedN("Rule.writeBack");
    // Find AABB of cells that actually changed so visual work stays sparse
    // (P4).
    int minX = width;
    int minY = height;
    int maxX = -1;
    int maxY = -1;
    bool anyChange = false;

    for (int y = 0; y < height; ++y) {
      const size_t rowBase =
        static_cast<size_t>(y) * static_cast<size_t>(width);
      for (int x = 0; x < width; ++x) {
        const size_t i = rowBase + static_cast<size_t>(x);
        if (src[i] != nextGen[i]) {
          anyChange = true;
          if (x < minX) {
            minX = x;
          }
          if (y < minY) {
            minY = y;
          }
          if (x > maxX) {
            maxX = x;
          }
          if (y > maxY) {
            maxY = y;
          }
        }
      }
    }

    if (anyChange) {
      std::memcpy(canvas->lifeCanvas, nextGen.data(), total);
      canvas->markCellsDirtyRegion(minX, minY, maxX, maxY);
    }
  }
}
