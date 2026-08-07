#pragma once
#include "Foundation/MacroDefs.h"
#include <cstring>

// Inclusive axis-aligned cell region. Invalid when maxX < minX.
struct DirtyRect
{
  int minX;
  int minY;
  int maxX;
  int maxY;

  void clear()
  {
    minX = 0;
    minY = 0;
    maxX = -1;
    maxY = -1;
  }

  bool valid() const { return maxX >= minX && maxY >= minY; }

  void setFull(int width, int height)
  {
    if (width <= 0 || height <= 0) {
      clear();
      return;
    }
    minX = 0;
    minY = 0;
    maxX = width - 1;
    maxY = height - 1;
  }

  void include(int x, int y)
  {
    if (!valid()) {
      minX = maxX = x;
      minY = maxY = y;
      return;
    }
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

  void includeRect(int x0, int y0, int x1, int y1)
  {
    include(x0, y0);
    include(x1, y1);
  }

  int width() const { return valid() ? (maxX - minX + 1) : 0; }
  int height() const { return valid() ? (maxY - minY + 1) : 0; }
};

// Pure simulation domain: dense cell storage + dirty-region tracking.
// No Renderer, window, camera, textures, or render tokens (D-C2).
// Presentation lives on Canvas, which extends this type.
//
// Double-buffer: lifeCanvas is the current front generation; lifeCanvasBack is
// the write target for calcGeneration. swapLifeBuffers() exchanges them so a
// generation does not need a full-grid memcpy (D-P5).
class CellGrid
{
public:
  CellGrid(int width, int height);
  ~CellGrid();

  CellGrid(const CellGrid&) = delete;
  CellGrid& operator=(const CellGrid&) = delete;

  int getWidth() const { return canvasWidth; }
  int getHeight() const { return canvasHeight; }

  __ILLUMO_FORCE_INLINE__ bool inBounds(const int& x, const int& y) const
  {
    return x >= 0 && y >= 0 && x < canvasWidth && y < canvasHeight &&
           lifeCanvas != nullptr;
  }

  // Logical cell write. Marks life dirty for visual rebuild / next upload.
  __ILLUMO_FORCE_INLINE__ bool setCanvasPixel(const int& x,
                                              const int& y,
                                              const unsigned char& colorVal)
  {
    if (!inBounds(x, y)) {
      return false;
    }
    const int idx = canvasWidth * y + x;
    if (lifeCanvas[idx] != colorVal) {
      lifeCanvas[idx] = colorVal;
      cellsDirty = true;
      cellsDirtyRect.include(x, y);
    }
    return true;
  }

  __ILLUMO_FORCE_INLINE__ unsigned char getCanvasPixel(const int& x,
                                                       const int& y) const
  {
    if (!inBounds(x, y)) {
      return 1;
    }
    return lifeCanvas[canvasWidth * y + x];
  }

  // Domain clear: every cell becomes dead (value 1). No presentation state.
  void clearCells();

  void markCellsDirty();
  void markCellsDirtyRegion(int x0, int y0, int x1, int y1);
  bool isCellsDirty() const { return cellsDirty; }
  bool hasCellsDirtyRegion() const
  {
    return cellsDirty && cellsDirtyRect.valid();
  }
  const DirtyRect& getCellsDirtyRegion() const { return cellsDirtyRect; }

  // Back buffer for double-buffered generation write (same size as front).
  unsigned char* getLifeBackBuffer() const { return lifeCanvasBack; }

  // Promote the back buffer to current front after a completed generation.
  void swapLifeBuffers();

  // Public for RuleSet double-buffer write and tight sim loops.
  // lifeCanvas always points at the current front generation.
  int canvasWidth;
  int canvasHeight;
  unsigned char* lifeCanvas;

protected:
  unsigned char* lifeCanvasBack;
  bool cellsDirty;
  DirtyRect cellsDirtyRect;

  void freeLifeStorage();
  void allocateLifeStorage(int width, int height);
};
