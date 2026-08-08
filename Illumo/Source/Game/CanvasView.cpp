#include "CanvasView.h"
#include "IRenderWindow.h"
#include "Rendering/Camera.h"
#include "Rendering/Renderer.h"
#include "Rulesets/RuleSet.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <glm/glm.hpp>
#include <limits>

CanvasView::CanvasView(int width,
                       int height,
                       SparseCellGrid* targetGrid,
                       IRenderWindow* renderWindow,
                       Camera* renderCamera,
                       Renderer* renderRenderer)
  : window(renderWindow)
  , camera(renderCamera)
  , renderer(renderRenderer)
  , viewWidth(width < 1 ? 1 : width)
  , viewHeight(height < 1 ? 1 : height)
  , grid(targetGrid)
  , texBuffer(nullptr)
  , displayRgb(nullptr)
  , targetRgb(nullptr)
  , fadeSpeed(8.0f)
  , displayTextureHandle(0)
  , gpuReady(false)
  , fadeActive(false)
  , textureUploadPending(true)
  , uploadMinX(0)
  , uploadMinY(0)
  , uploadMaxX(this->viewWidth - 1)
  , uploadMaxY(this->viewHeight - 1)
  , lastWindowWidth(0)
  , lastWindowHeight(0)
  , screenQuadReady(false)
{
  const std::size_t texelCount = static_cast<std::size_t>(this->viewWidth) *
                                 static_cast<std::size_t>(this->viewHeight);
  texBuffer = new unsigned char[texelCount * 3u];
  displayRgb = new float[texelCount * 3u];
  targetRgb = new float[texelCount * 3u];
  visibleCells.resize(texelCount);
  const CellAddress invalid{ std::numeric_limits<std::int64_t>::max(),
                             std::numeric_limits<std::int64_t>::max() };
  for (CellAddress& address : visibleCells) {
    address = invalid;
  }
  for (std::size_t i = 0; i < texelCount * 3u; ++i) {
    texBuffer[i] = 255;
    displayRgb[i] = 1.0f;
    targetRgb[i] = 1.0f;
  }
  rebuildDefaultPalette();
  initializeGpuResources();
}

CanvasView::~CanvasView()
{
  delete[] texBuffer;
  delete[] displayRgb;
  delete[] targetRgb;
}

std::int64_t
CanvasView::worldToCell(double worldCoordinate)
{
  return static_cast<std::int64_t>(
    std::floor(worldCoordinate / static_cast<double>(kCellSize) + 0.5));
}

bool
CanvasView::sameAddress(const CellAddress& left, const CellAddress& right)
{
  return left.x == right.x && left.y == right.y;
}

void
CanvasView::rebuildDefaultPalette()
{
  for (int state = 0; state < kPaletteSize; ++state) {
    const int base = state * 3;
    if (state == SparseCellGrid::CountedNeighborState) {
      paletteRgb[base + 0] = 0;
      paletteRgb[base + 1] = 0;
      paletteRgb[base + 2] = 0;
    } else if (state == SparseCellGrid::BackgroundState) {
      paletteRgb[base + 0] = 255;
      paletteRgb[base + 1] = 255;
      paletteRgb[base + 2] = 255;
    } else {
      paletteRgb[base + 0] = 0;
      paletteRgb[base + 1] = 164;
      paletteRgb[base + 2] = 128;
    }
  }
}

void
CanvasView::rebuildPalette(const RuleSet* rules)
{
  if (rules == nullptr) {
    rebuildDefaultPalette();
  } else {
    for (int state = 0; state < kPaletteSize; ++state) {
      unsigned char rgb[3] = { 255, 255, 255 };
      rules->evalCell(static_cast<unsigned char>(state), rgb);
      const int base = state * 3;
      paletteRgb[base + 0] = rgb[0];
      paletteRgb[base + 1] = rgb[1];
      paletteRgb[base + 2] = rgb[2];
    }
  }
  rebuildTargetsFromGrid();
}

void
CanvasView::initializeGpuResources()
{
  if (renderer == nullptr) {
    return;
  }
  renderer->ensureBuiltinStyles();
  visual.setRenderer(renderer);
  visual.setWindow(window);
  visual.setCamera(camera);
  visual.setSpace(PrimitiveSpace::Pixels);
  visual.setLayerHint(RenderLayerId::World);
  visual.prepare(renderer);
  displayTextureHandle = renderer->allocateHandle();
  renderer->enrollTexture(texBuffer,
                          viewWidth,
                          viewHeight,
                          3,
                          displayTextureHandle,
                          TextureFilter::Linear);
  gpuReady = true;
  rebuildScreenQuad();
}

void
CanvasView::rebuildScreenQuad()
{
  int width = 1280;
  int height = 720;
  if (window != nullptr) {
    std::array<int, 2> dimensions = window->getWindowDimensions();
    width = dimensions[0];
    height = dimensions[1];
  }
  if (width == lastWindowWidth && height == lastWindowHeight &&
      screenQuadReady) {
    return;
  }
  lastWindowWidth = width;
  lastWindowHeight = height;
  visual.clearPrimitives();
  visual.addSprite(displayTextureHandle,
                   0.0f,
                   0.0f,
                   static_cast<float>(width),
                   static_cast<float>(height),
                   ColorRgba{ 255, 255, 255, 255 });
  screenQuadReady = true;
}

void
CanvasView::includeUpload(int x, int y)
{
  textureUploadPending = true;
  if (uploadMaxX < uploadMinX || uploadMaxY < uploadMinY) {
    uploadMinX = x;
    uploadMaxX = x;
    uploadMinY = y;
    uploadMaxY = y;
    return;
  }
  uploadMinX = std::min(uploadMinX, x);
  uploadMinY = std::min(uploadMinY, y);
  uploadMaxX = std::max(uploadMaxX, x);
  uploadMaxY = std::max(uploadMaxY, y);
}

void
CanvasView::writeTexel(int index, int x, int y)
{
  const int base = index * 3;
  bool changed = false;
  for (int channel = 0; channel < 3; ++channel) {
    const float scaled = displayRgb[base + channel] * 255.0f + 0.5f;
    const unsigned char value =
      static_cast<unsigned char>(std::clamp(scaled, 0.0f, 255.0f));
    if (texBuffer[base + channel] != value) {
      texBuffer[base + channel] = value;
      changed = true;
    }
  }
  if (changed) {
    includeUpload(x, y);
  }
}

void
CanvasView::setTargetForSlot(int index,
                             unsigned char r,
                             unsigned char g,
                             unsigned char b,
                             bool snap)
{
  const int base = index * 3;
  const float targetR = static_cast<float>(r) / 255.0f;
  const float targetG = static_cast<float>(g) / 255.0f;
  const float targetB = static_cast<float>(b) / 255.0f;
  const bool changed = targetRgb[base + 0] != targetR ||
                       targetRgb[base + 1] != targetG ||
                       targetRgb[base + 2] != targetB;
  targetRgb[base + 0] = targetR;
  targetRgb[base + 1] = targetG;
  targetRgb[base + 2] = targetB;
  if (snap) {
    displayRgb[base + 0] = targetR;
    displayRgb[base + 1] = targetG;
    displayRgb[base + 2] = targetB;
  } else if (changed) {
    fadeActive = true;
  }
}

void
CanvasView::syncVisibleRegion()
{
  int width = 1280;
  int height = 720;
  double zoom = 1.0;
  glm::dvec2 position(0.0, 0.0);
  if (window != nullptr) {
    std::array<int, 2> dimensions = window->getWindowDimensions();
    width = dimensions[0];
    height = dimensions[1];
  }
  if (camera != nullptr) {
    zoom = std::max(0.1, static_cast<double>(camera->GetZoom()));
    position = camera->GetPositionPrecise();
  }
  rebuildScreenQuad();

  const double worldWidth = static_cast<double>(width) / zoom;
  const double worldHeight = static_cast<double>(height) / zoom;
  const double worldLeft = position.x - worldWidth * 0.5;
  const double worldTop = position.y + worldHeight * 0.5;
  std::vector<CellAddress> nextVisibleCells(visibleCells.size());
  for (int y = 0; y < viewHeight; ++y) {
    for (int x = 0; x < viewWidth; ++x) {
      const double sampleX = worldLeft + (static_cast<double>(x) + 0.5) *
                                           worldWidth /
                                           static_cast<double>(viewWidth);
      const double sampleY = worldTop - (static_cast<double>(y) + 0.5) *
                                          worldHeight /
                                          static_cast<double>(viewHeight);
      nextVisibleCells[static_cast<std::size_t>(y * viewWidth + x)] =
        CellAddress{ worldToCell(sampleX), worldToCell(sampleY) };
    }
  }

  bool mappingChanged = false;
  for (std::size_t i = 0; i < visibleCells.size(); ++i) {
    if (!sameAddress(visibleCells[i], nextVisibleCells[i])) {
      mappingChanged = true;
      break;
    }
  }
  visibleCells.swap(nextVisibleCells);

  if (mappingChanged && grid != nullptr) {
    for (int y = 0; y < viewHeight; ++y) {
      for (int x = 0; x < viewWidth; ++x) {
        const int index = y * viewWidth + x;
        const unsigned char state =
          grid->getCell(visibleCells[static_cast<std::size_t>(index)]);
        const int paletteIndex = static_cast<int>(state) * 3;
        setTargetForSlot(index,
                         paletteRgb[paletteIndex + 0],
                         paletteRgb[paletteIndex + 1],
                         paletteRgb[paletteIndex + 2],
                         true);
        writeTexel(index, x, y);
      }
    }
    fadeActive = false;
    uploadMinX = 0;
    uploadMinY = 0;
    uploadMaxX = viewWidth - 1;
    uploadMaxY = viewHeight - 1;
    textureUploadPending = true;
  }
}

void
CanvasView::rebuildTargetsFromGrid()
{
  syncVisibleRegion();
  if (grid == nullptr) {
    return;
  }
  for (int y = 0; y < viewHeight; ++y) {
    for (int x = 0; x < viewWidth; ++x) {
      const int index = y * viewWidth + x;
      const unsigned char state =
        grid->getCell(visibleCells[static_cast<std::size_t>(index)]);
      const int paletteIndex = static_cast<int>(state) * 3;
      setTargetForSlot(index,
                       paletteRgb[paletteIndex + 0],
                       paletteRgb[paletteIndex + 1],
                       paletteRgb[paletteIndex + 2],
                       false);
    }
  }
  if (fadeSpeed <= 0.0f) {
    snapVisualToTargets();
  }
}

void
CanvasView::clearView()
{
  if (grid != nullptr) {
    grid->clear();
  }
  rebuildTargetsFromGrid();
  snapVisualToTargets();
}

void
CanvasView::setFadeSpeed(float speed)
{
  fadeSpeed = std::max(0.0f, speed);
  if (fadeSpeed <= 0.0f) {
    snapVisualToTargets();
  }
}

void
CanvasView::snapVisualToTargets()
{
  for (int y = 0; y < viewHeight; ++y) {
    for (int x = 0; x < viewWidth; ++x) {
      const int index = y * viewWidth + x;
      const int base = index * 3;
      displayRgb[base + 0] = targetRgb[base + 0];
      displayRgb[base + 1] = targetRgb[base + 1];
      displayRgb[base + 2] = targetRgb[base + 2];
      writeTexel(index, x, y);
    }
  }
  fadeActive = false;
}

void
CanvasView::tickVisual(float dt)
{
  if (!fadeActive) {
    return;
  }
  if (dt < 0.0f) {
    dt = 0.0f;
  }
  if (fadeSpeed <= 0.0f) {
    snapVisualToTargets();
    return;
  }
  const float alpha = std::min(1.0f, 1.0f - std::exp(-fadeSpeed * dt));
  bool stillFading = false;
  for (int y = 0; y < viewHeight; ++y) {
    for (int x = 0; x < viewWidth; ++x) {
      const int index = y * viewWidth + x;
      const int base = index * 3;
      bool cellStillFading = false;
      for (int channel = 0; channel < 3; ++channel) {
        const float target = targetRgb[base + channel];
        const float difference = target - displayRgb[base + channel];
        if (std::fabs(difference) > 0.002f) {
          displayRgb[base + channel] += difference * alpha;
          if (std::fabs(target - displayRgb[base + channel]) > 0.002f) {
            cellStillFading = true;
          } else {
            displayRgb[base + channel] = target;
          }
        } else {
          displayRgb[base + channel] = target;
        }
      }
      stillFading = stillFading || cellStillFading;
      writeTexel(index, x, y);
    }
  }
  fadeActive = stillFading;
}

CellAddress
CanvasView::getVisibleCell(int x, int y) const
{
  if (x < 0 || y < 0 || x >= viewWidth || y >= viewHeight) {
    return CellAddress{ std::numeric_limits<std::int64_t>::max(),
                        std::numeric_limits<std::int64_t>::max() };
  }
  return visibleCells[static_cast<std::size_t>(y * viewWidth + x)];
}

void
CanvasView::DrawImpl()
{
}

bool
CanvasView::AppendCommands(Renderer* activeRenderer)
{
  if (!isVisible() || !gpuReady || activeRenderer == nullptr) {
    return isVisible();
  }
  rebuildScreenQuad();
  if (textureUploadPending) {
    int x = std::max(0, uploadMinX);
    int y = std::max(0, uploadMinY);
    if (x >= viewWidth || y >= viewHeight) {
      x = 0;
      y = 0;
    }
    int width = uploadMaxX - x + 1;
    int height = uploadMaxY - y + 1;
    if (width <= 0 || height <= 0) {
      x = 0;
      y = 0;
      width = viewWidth;
      height = viewHeight;
    }
    activeRenderer->pushUpdateTexture(
      displayTextureHandle,
      x,
      y,
      std::min(width, viewWidth - x),
      std::min(height, viewHeight - y),
      3,
      texBuffer +
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(viewWidth) +
         static_cast<std::size_t>(x)) *
          3u,
      viewWidth);
    textureUploadPending = false;
    uploadMinX = viewWidth;
    uploadMinY = viewHeight;
    uploadMaxX = -1;
    uploadMaxY = -1;
  }
  visual.setRenderer(activeRenderer);
  visual.setVisible(isVisible());
  return visual.AppendCommands(activeRenderer);
}
