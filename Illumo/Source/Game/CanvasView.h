#pragma once

#include "Game/SparseCellGrid.h"
#include "Rendering/Drawable.h"
#include "Rendering/Primitives/GameVisual.h"
#include <cstdint>
#include <vector>

class Camera;
class IRenderWindow;
class Renderer;
class RuleSet;

// Bounded presentation of an unbounded SparseCellGrid. The view owns one
// reusable RGB staging texture and one screen-space quad; simulation chunks
// never become individual GPU resources or render commands.
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

  int getViewWidth() const { return viewWidth; }
  int getViewHeight() const { return viewHeight; }
  SparseCellGrid* getGrid() const { return grid; }

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

  // Kept public for the small headless fixture and diagnostics.
  IRenderWindow* window;
  Camera* camera;
  Renderer* renderer;

private:
  static const int kPaletteSize = 256;
  static constexpr float kCellSize = 16.0f;

  int viewWidth;
  int viewHeight;
  SparseCellGrid* grid;
  unsigned char paletteRgb[kPaletteSize * 3];
  unsigned char* texBuffer;
  float* displayRgb;
  float* targetRgb;
  std::vector<CellAddress> visibleCells;
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
  int lastWindowWidth;
  int lastWindowHeight;
  bool screenQuadReady;

  static std::int64_t worldToCell(double worldCoordinate);
  static bool sameAddress(const CellAddress& left, const CellAddress& right);
  void initializeGpuResources();
  void rebuildScreenQuad();
  void includeUpload(int x, int y);
  void writeTexel(int index, int x, int y);
  void setTargetForSlot(int index,
                        unsigned char r,
                        unsigned char g,
                        unsigned char b,
                        bool snap);
};
