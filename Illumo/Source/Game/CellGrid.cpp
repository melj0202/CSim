#include "CellGrid.h"

CellGrid::CellGrid(int width, int height)
  : canvasWidth(0)
  , canvasHeight(0)
  , lifeCanvas(nullptr)
  , lifeCanvasBack(nullptr)
  , cellsDirty(true)
{
  cellsDirtyRect.clear();
  allocateLifeStorage(width, height);
}

CellGrid::~CellGrid()
{
  freeLifeStorage();
}

void
CellGrid::allocateLifeStorage(int width, int height)
{
  freeLifeStorage();
  if (width < 1) {
    width = 1;
  }
  if (height < 1) {
    height = 1;
  }
  canvasWidth = width;
  canvasHeight = height;
  const size_t total =
    static_cast<size_t>(canvasWidth) * static_cast<size_t>(canvasHeight);
  lifeCanvas = new unsigned char[total];
  lifeCanvasBack = new unsigned char[total];
  std::memset(lifeCanvas, 1, total);
  std::memset(lifeCanvasBack, 1, total);
  cellsDirty = true;
  cellsDirtyRect.setFull(canvasWidth, canvasHeight);
}

void
CellGrid::freeLifeStorage()
{
  delete[] lifeCanvas;
  delete[] lifeCanvasBack;
  lifeCanvas = nullptr;
  lifeCanvasBack = nullptr;
  canvasWidth = 0;
  canvasHeight = 0;
  cellsDirty = false;
  cellsDirtyRect.clear();
}

void
CellGrid::swapLifeBuffers()
{
  unsigned char* tmp = lifeCanvas;
  lifeCanvas = lifeCanvasBack;
  lifeCanvasBack = tmp;
}

void
CellGrid::clearCells()
{
  if (lifeCanvas == nullptr || canvasWidth <= 0 || canvasHeight <= 0) {
    return;
  }
  const size_t total =
    static_cast<size_t>(canvasWidth) * static_cast<size_t>(canvasHeight);
  std::memset(lifeCanvas, 1, total);
  // Keep back buffer consistent so a later swap cannot resurrect stale cells.
  if (lifeCanvasBack != nullptr) {
    std::memset(lifeCanvasBack, 1, total);
  }
  cellsDirty = true;
  cellsDirtyRect.setFull(canvasWidth, canvasHeight);
}

void
CellGrid::markCellsDirty()
{
  cellsDirty = true;
  cellsDirtyRect.setFull(canvasWidth, canvasHeight);
}

void
CellGrid::markCellsDirtyRegion(int x0, int y0, int x1, int y1)
{
  if (x0 > x1) {
    const int t = x0;
    x0 = x1;
    x1 = t;
  }
  if (y0 > y1) {
    const int t = y0;
    y0 = y1;
    y1 = t;
  }
  if (x0 < 0) {
    x0 = 0;
  }
  if (y0 < 0) {
    y0 = 0;
  }
  if (x1 >= canvasWidth) {
    x1 = canvasWidth - 1;
  }
  if (y1 >= canvasHeight) {
    y1 = canvasHeight - 1;
  }
  if (x0 > x1 || y0 > y1) {
    return;
  }
  cellsDirty = true;
  cellsDirtyRect.includeRect(x0, y0, x1, y1);
}
