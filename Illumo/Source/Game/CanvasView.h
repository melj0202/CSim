#pragma once

#include "Game/SparseCellGrid.h"
#include "Rendering/Drawable.h"
#include "Rendering/Primitives/GameVisual.h"
#include <cstddef>
#include <cstdint>
#include <vector>

class Camera;
class IRenderWindow;
class Renderer;
class RuleSet;

// Bounded presentation of an unbounded SparseCellGrid. The view owns one
// reusable RGB staging texture and one world-space, cell-aligned quad;
// simulation chunks never become individual GPU resources or render commands.
class CanvasView : public Drawable<CanvasView>
{
public:
  CanvasView(int viewWidth,
             int viewHeight,
             SparseCellGrid* grid,
             IRenderWindow* window = nullptr,
             Camera* camera = nullptr,
             Renderer* renderer = nullptr);
  ~CanvasView();

  CanvasView(const CanvasView&) = delete;
  CanvasView& operator=(const CanvasView&) = delete;

  // Active display texels. These are one-to-one with cells near the camera and
  // become a bounded density overview only when zooming far out.
  int getViewWidth() const { return activeViewWidth; }
  int getViewHeight() const { return activeViewHeight; }
  int getTextureWidth() const { return textureWidth; }
  int getTextureHeight() const { return textureHeight; }
  int getVisibleCellWidth() const { return visibleCellWidth; }
  int getVisibleCellHeight() const { return visibleCellHeight; }
  CellAddress getVisibleFirstCell() const { return visibleFirstCell; }
  SparseCellGrid* getGrid() const { return grid; }

  static std::int64_t worldToCell(double worldCoordinate);

  void clearView();
  void clearCanvas() { clearView(); }
  unsigned char getCanvasPixel(std::int64_t x, std::int64_t y) const
  {
    return grid == nullptr ? SparseCellGrid::BackgroundState
                           : grid->getCell(CellAddress{ x, y });
  }
  bool setCanvasPixel(std::int64_t x, std::int64_t y, unsigned char state)
  {
    return grid != nullptr && grid->setCell(CellAddress{ x, y }, state);
  }
  void syncVisibleRegion();
  void rebuildTargetsFromGrid();
  void rebuildPalette(const RuleSet* rules);
  void rebuildDefaultPalette();
  void setFadeSpeed(float speed);
  float getFadeSpeed() const { return fadeSpeed; }
  void tickVisual(float dt);
  void snapVisualToTargets();

  void DrawImpl();
  bool AppendCommands(Renderer* renderer) override;

  GameVisual& getVisual() { return visual; }
  const GameVisual& getVisual() const { return visual; }

  CellAddress getVisibleCell(int x, int y) const;
  const unsigned char* getDisplayTexBuffer() const { return texBuffer; }
  const unsigned char* getPaletteRgb() const { return paletteRgb; }
  bool isFadeActive() const { return fadeActive; }
  bool isTextureUploadPending() const { return textureUploadPending; }
  std::size_t getFadingTexelCount() const { return fadingTexels.size(); }
  std::size_t getLastSampledTexelCount() const { return lastSampledTexelCount; }
  std::size_t getLastFadeVisitCount() const { return lastFadeVisitCount; }
  std::size_t getLastSnapVisitCountForTesting() const
  {
    return lastSnapVisitCount;
  }

  // Kept public for the small headless fixture and diagnostics.
  IRenderWindow* window;
  Camera* camera;
  Renderer* renderer;

private:
  static const int kPaletteSize = 256;
  static constexpr float kCellSize = 16.0f;
  static const int kOverviewPixelsPerTexel = 4;

  int baseViewWidth;
  int baseViewHeight;
  int textureWidth;
  int textureHeight;
  int activeViewWidth;
  int activeViewHeight;
  int visibleCellWidth;
  int visibleCellHeight;
  CellAddress visibleFirstCell;
  SparseCellGrid* grid;
  unsigned char paletteRgb[kPaletteSize * 3];
  unsigned char* texBuffer;
  float* displayRgb;
  float* targetRgb;
  float* sampledRgb;
  std::vector<int> fadingTexels;
  std::vector<unsigned char> fadingFlags;
  std::size_t lastSampledTexelCount;
  std::size_t lastFadeVisitCount;
  std::size_t lastSnapVisitCount;
  float fadeSpeed;

  GameVisual visual;
  unsigned long displayTextureHandle;
  bool gpuReady;
  bool fadeActive;
  bool textureUploadPending;
  int uploadMinX;
  int uploadMinY;
  int uploadMaxX;
  int uploadMaxY;
  CellAddress quadFirstCell;
  int quadCellWidth;
  int quadCellHeight;
  int quadActiveWidth;
  int quadActiveHeight;
  std::uint64_t lastGridRevision;
  bool regionReady;
  bool paletteDirty;
  bool worldQuadReady;

  static bool sameAddress(const CellAddress& left, const CellAddress& right);
  static int growTextureDimension(int current, int required);
  void initializeGpuResources();
  void resizeBuffers(int width, int height);
  void resetUploadBounds();
  void markFullActiveUpload();
  void rebuildWorldQuad();
  void sampleGrid(bool snap);
  bool sampleChangedChunks(std::uint64_t previousRevision);
  void applySampledTargets(bool snap);
  void clearFadingTexels();
  int getSlotSampleCount(int x, int y) const;
  void includeUpload(int x, int y);
  void writeTexel(int index, int x, int y);
  void setTargetForSlot(int index, float r, float g, float b, bool snap);
};
