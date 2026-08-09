#include "CanvasView.h"
#include "IRenderWindow.h"
#include "Rendering/Camera.h"
#include "Rendering/Renderer.h"
#include "Rulesets/RuleSet.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <glm/glm.hpp>
#include <limits>
#include <tracy/Tracy.hpp>

CanvasView::CanvasView(int width,
                       int height,
                       SparseCellGrid* targetGrid,
                       IRenderWindow* renderWindow,
                       Camera* renderCamera,
                       Renderer* renderRenderer)
  : window(renderWindow)
  , camera(renderCamera)
  , renderer(renderRenderer)
  , baseViewWidth(width < 1 ? 1 : width)
  , baseViewHeight(height < 1 ? 1 : height)
  , textureWidth(baseViewWidth)
  , textureHeight(baseViewHeight)
  , activeViewWidth(0)
  , activeViewHeight(0)
  , visibleCellWidth(0)
  , visibleCellHeight(0)
  , visibleFirstCell{ 0, 0 }
  , grid(targetGrid)
  , texBuffer(nullptr)
  , displayRgb(nullptr)
  , targetRgb(nullptr)
  , sampledRgb(nullptr)
  , fadeSpeed(8.0f)
  , displayTextureHandle()
  , gpuReady(false)
  , fadeActive(false)
  , textureUploadPending(false)
  , uploadMinX(0)
  , uploadMinY(0)
  , uploadMaxX(-1)
  , uploadMaxY(-1)
  , quadFirstCell{ 0, 0 }
  , quadCellWidth(0)
  , quadCellHeight(0)
  , quadActiveWidth(0)
  , quadActiveHeight(0)
  , lastGridRevision(std::numeric_limits<std::uint64_t>::max())
  , regionReady(false)
  , paletteDirty(true)
  , worldQuadReady(false)
{
  const std::size_t texelCount = static_cast<std::size_t>(textureWidth) *
                                 static_cast<std::size_t>(textureHeight);
  texBuffer = new unsigned char[texelCount * 3u];
  displayRgb = new float[texelCount * 3u];
  targetRgb = new float[texelCount * 3u];
  sampledRgb = new float[texelCount * 3u];
  for (std::size_t i = 0; i < texelCount * 3u; ++i) {
    texBuffer[i] = 255;
    displayRgb[i] = 1.0f;
    targetRgb[i] = 1.0f;
    sampledRgb[i] = 1.0f;
  }
  rebuildDefaultPalette();
  resetUploadBounds();
  initializeGpuResources();
}

CanvasView::~CanvasView()
{
  if (renderer != nullptr && displayTextureHandle.isValid()) {
    renderer->destroyTexture(displayTextureHandle);
    displayTextureHandle = TextureHandle{};
  }
  delete[] texBuffer;
  delete[] displayRgb;
  delete[] targetRgb;
  delete[] sampledRgb;
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
  paletteDirty = true;
}

void
CanvasView::rebuildPalette(const RuleSet* rules)
{
  ZoneScopedN("CanvasView.rebuildPalette");
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
    paletteDirty = true;
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
  visual.setSpace(PrimitiveSpace::World);
  visual.setLayerHint(RenderLayerId::World);
  visual.prepare(renderer);
  TextureOptions textureOptions;
  textureOptions.filter = TextureFilter::Nearest;
  displayTextureHandle = renderer->enrollTexture(
    texBuffer, textureWidth, textureHeight, 3, textureOptions);
  gpuReady = true;
}

void
CanvasView::resizeBuffers(int width, int height)
{
  if (width <= textureWidth && height <= textureHeight) {
    return;
  }

  const int newWidth = std::max(textureWidth, width);
  const int newHeight = std::max(textureHeight, height);
  const std::size_t texelCount =
    static_cast<std::size_t>(newWidth) * static_cast<std::size_t>(newHeight);

  delete[] texBuffer;
  delete[] displayRgb;
  delete[] targetRgb;
  delete[] sampledRgb;

  textureWidth = newWidth;
  textureHeight = newHeight;
  texBuffer = new unsigned char[texelCount * 3u];
  displayRgb = new float[texelCount * 3u];
  targetRgb = new float[texelCount * 3u];
  sampledRgb = new float[texelCount * 3u];
  for (std::size_t i = 0; i < texelCount * 3u; ++i) {
    texBuffer[i] = 255;
    displayRgb[i] = 1.0f;
    targetRgb[i] = 1.0f;
    sampledRgb[i] = 1.0f;
  }

  fadeActive = false;
  textureUploadPending = false;
  resetUploadBounds();
  regionReady = false;
  worldQuadReady = false;
  lastGridRevision = std::numeric_limits<std::uint64_t>::max();

  if (gpuReady && renderer != nullptr) {
    TextureOptions textureOptions;
    textureOptions.filter = TextureFilter::Nearest;
    renderer->replaceTexture(displayTextureHandle,
                             texBuffer,
                             textureWidth,
                             textureHeight,
                             3,
                             textureOptions);
  }
}

void
CanvasView::resetUploadBounds()
{
  uploadMinX = textureWidth;
  uploadMinY = textureHeight;
  uploadMaxX = -1;
  uploadMaxY = -1;
}

void
CanvasView::markFullActiveUpload()
{
  if (activeViewWidth <= 0 || activeViewHeight <= 0) {
    return;
  }
  textureUploadPending = true;
  uploadMinX = 0;
  uploadMinY = 0;
  uploadMaxX = activeViewWidth - 1;
  uploadMaxY = activeViewHeight - 1;
}

void
CanvasView::rebuildWorldQuad()
{
  if (worldQuadReady && sameAddress(quadFirstCell, visibleFirstCell) &&
      quadCellWidth == visibleCellWidth &&
      quadCellHeight == visibleCellHeight &&
      quadActiveWidth == activeViewWidth &&
      quadActiveHeight == activeViewHeight) {
    return;
  }

  const float worldLeft =
    static_cast<float>(visibleFirstCell.x) * kCellSize - kCellSize * 0.5f;
  const float worldTop =
    static_cast<float>(visibleFirstCell.y) * kCellSize + kCellSize * 0.5f;
  const float worldBottom =
    worldTop - static_cast<float>(visibleCellHeight) * kCellSize;
  const float u1 =
    static_cast<float>(activeViewWidth) / static_cast<float>(textureWidth);
  const float v0 =
    static_cast<float>(activeViewHeight) / static_cast<float>(textureHeight);
  visual.clearPrimitives();
  visual.addSprite(displayTextureHandle,
                   worldLeft,
                   worldBottom,
                   static_cast<float>(visibleCellWidth) * kCellSize,
                   static_cast<float>(visibleCellHeight) * kCellSize,
                   ColorRgba{ 255, 255, 255, 255 },
                   0.0f,
                   v0,
                   u1,
                   0.0f);
  quadFirstCell = visibleFirstCell;
  quadCellWidth = visibleCellWidth;
  quadCellHeight = visibleCellHeight;
  quadActiveWidth = activeViewWidth;
  quadActiveHeight = activeViewHeight;
  worldQuadReady = true;
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
CanvasView::setTargetForSlot(int index, float r, float g, float b, bool snap)
{
  const int base = index * 3;
  const bool changed = targetRgb[base + 0] != r || targetRgb[base + 1] != g ||
                       targetRgb[base + 2] != b;
  targetRgb[base + 0] = r;
  targetRgb[base + 1] = g;
  targetRgb[base + 2] = b;
  if (snap) {
    displayRgb[base + 0] = r;
    displayRgb[base + 1] = g;
    displayRgb[base + 2] = b;
  } else if (changed) {
    fadeActive = true;
  }
}

int
CanvasView::getSlotSampleCount(int x, int y) const
{
  const int sourceLeft = x * visibleCellWidth / activeViewWidth;
  const int sourceRight = (x + 1) * visibleCellWidth / activeViewWidth;
  const int sourceTop = y * visibleCellHeight / activeViewHeight;
  const int sourceBottom = (y + 1) * visibleCellHeight / activeViewHeight;
  return (sourceRight - sourceLeft) * (sourceBottom - sourceTop);
}

void
CanvasView::applySampledTargets(bool snap)
{
  for (int y = 0; y < activeViewHeight; ++y) {
    for (int x = 0; x < activeViewWidth; ++x) {
      const int index = y * textureWidth + x;
      const int base = index * 3;
      setTargetForSlot(index,
                       sampledRgb[base + 0],
                       sampledRgb[base + 1],
                       sampledRgb[base + 2],
                       snap);
      if (snap) {
        writeTexel(index, x, y);
      }
    }
  }
  if (snap) {
    fadeActive = false;
    markFullActiveUpload();
  }
}

void
CanvasView::sampleGrid(bool snap)
{
  ZoneScopedN("CanvasView.sampleGrid");
  if (activeViewWidth <= 0 || activeViewHeight <= 0) {
    return;
  }

  const int backgroundBase = SparseCellGrid::BackgroundState * 3;
  const float backgroundR =
    static_cast<float>(paletteRgb[backgroundBase + 0]) / 255.0f;
  const float backgroundG =
    static_cast<float>(paletteRgb[backgroundBase + 1]) / 255.0f;
  const float backgroundB =
    static_cast<float>(paletteRgb[backgroundBase + 2]) / 255.0f;
  for (int y = 0; y < activeViewHeight; ++y) {
    for (int x = 0; x < activeViewWidth; ++x) {
      const int index = y * textureWidth + x;
      const int base = index * 3;
      sampledRgb[base + 0] = backgroundR;
      sampledRgb[base + 1] = backgroundG;
      sampledRgb[base + 2] = backgroundB;
    }
  }

  if (grid != nullptr) {
    const std::int64_t maximumX =
      visibleFirstCell.x + static_cast<std::int64_t>(visibleCellWidth) - 1;
    const std::int64_t minimumY =
      visibleFirstCell.y - static_cast<std::int64_t>(visibleCellHeight) + 1;
    const ChunkAddress minimumChunk = SparseCellGrid::chunkAddressForCell(
      CellAddress{ visibleFirstCell.x, minimumY });
    const ChunkAddress maximumChunk = SparseCellGrid::chunkAddressForCell(
      CellAddress{ maximumX, visibleFirstCell.y });
    grid->visitChunksInBounds(
      minimumChunk,
      maximumChunk,
      [this, maximumX, minimumY, backgroundR, backgroundG, backgroundB](
        const ChunkAddress& chunkAddress,
        const SparseCellGrid::ChunkCells& cells) {
        ZoneScopedN("CanvasView.accumulateChunk");
        for (int localY = 0; localY < SparseCellGrid::kChunkDim; ++localY) {
          const std::int64_t cellY =
            chunkAddress.y * SparseCellGrid::kChunkDim + localY;
          if (cellY < minimumY || cellY > visibleFirstCell.y) {
            continue;
          }
          for (int localX = 0; localX < SparseCellGrid::kChunkDim; ++localX) {
            const std::int64_t cellX =
              chunkAddress.x * SparseCellGrid::kChunkDim + localX;
            if (cellX < visibleFirstCell.x || cellX > maximumX) {
              continue;
            }
            const unsigned char state = cells[static_cast<std::size_t>(
              localY * SparseCellGrid::kChunkDim + localX)];
            if (state == SparseCellGrid::BackgroundState) {
              continue;
            }
            const int outputX =
              static_cast<int>((cellX - visibleFirstCell.x) * activeViewWidth /
                               visibleCellWidth);
            const int outputY =
              static_cast<int>((visibleFirstCell.y - cellY) * activeViewHeight /
                               visibleCellHeight);
            const int sampleCount = getSlotSampleCount(outputX, outputY);
            const int index = outputY * textureWidth + outputX;
            const int base = index * 3;
            const int paletteIndex = static_cast<int>(state) * 3;
            const float inverseSampleCount = 1.0f / sampleCount;
            sampledRgb[base + 0] +=
              (static_cast<float>(paletteRgb[paletteIndex + 0]) / 255.0f -
               backgroundR) *
              inverseSampleCount;
            sampledRgb[base + 1] +=
              (static_cast<float>(paletteRgb[paletteIndex + 1]) / 255.0f -
               backgroundG) *
              inverseSampleCount;
            sampledRgb[base + 2] +=
              (static_cast<float>(paletteRgb[paletteIndex + 2]) / 255.0f -
               backgroundB) *
              inverseSampleCount;
          }
        }
      });
  }
  applySampledTargets(snap);
}

void
CanvasView::syncVisibleRegion()
{
  ZoneScopedN("CanvasView.syncVisibleRegion");
  int width = 1280;
  int height = 720;
  double zoom = 1.0;
  glm::dvec2 position(0.0, 0.0);
  if (window != nullptr) {
    const std::array<int, 2> dimensions = window->getWindowDimensions();
    width = std::max(1, dimensions[0]);
    height = std::max(1, dimensions[1]);
  }
  if (camera != nullptr) {
    zoom = std::max(0.1, static_cast<double>(camera->GetZoom()));
    position = camera->GetPositionPrecise();
  }

  const double worldWidth = static_cast<double>(width) / zoom;
  const double worldHeight = static_cast<double>(height) / zoom;
  const int nextCellWidth =
    static_cast<int>(std::ceil(worldWidth / static_cast<double>(kCellSize))) +
    2;
  const int nextCellHeight =
    static_cast<int>(std::ceil(worldHeight / static_cast<double>(kCellSize))) +
    2;
  const int outputBudgetWidth =
    std::max(baseViewWidth,
             (width + kOverviewPixelsPerTexel - 1) / kOverviewPixelsPerTexel);
  const int outputBudgetHeight =
    std::max(baseViewHeight,
             (height + kOverviewPixelsPerTexel - 1) / kOverviewPixelsPerTexel);
  const int nextActiveWidth = std::min(nextCellWidth, outputBudgetWidth);
  const int nextActiveHeight = std::min(nextCellHeight, outputBudgetHeight);
  const std::int64_t centerX = worldToCell(position.x);
  const std::int64_t centerY = worldToCell(position.y);
  const CellAddress nextFirstCell{ centerX - nextCellWidth / 2,
                                   centerY + (nextCellHeight - 1) / 2 };
  const bool changed =
    !regionReady || !sameAddress(visibleFirstCell, nextFirstCell) ||
    visibleCellWidth != nextCellWidth || visibleCellHeight != nextCellHeight ||
    activeViewWidth != nextActiveWidth || activeViewHeight != nextActiveHeight;
  if (!changed) {
    return;
  }

  resizeBuffers(nextActiveWidth, nextActiveHeight);
  visibleFirstCell = nextFirstCell;
  visibleCellWidth = nextCellWidth;
  visibleCellHeight = nextCellHeight;
  activeViewWidth = nextActiveWidth;
  activeViewHeight = nextActiveHeight;
  regionReady = true;
  rebuildWorldQuad();
  if (grid != nullptr) {
    sampleGrid(true);
    lastGridRevision = grid->getRevision();
    paletteDirty = false;
  }
}

void
CanvasView::rebuildTargetsFromGrid()
{
  ZoneScopedN("CanvasView.rebuildTargetsFromGrid");
  syncVisibleRegion();
  if (grid == nullptr) {
    return;
  }
  const std::uint64_t currentRevision = grid->getRevision();
  if (!paletteDirty && currentRevision == lastGridRevision) {
    return;
  }
  sampleGrid(false);
  lastGridRevision = currentRevision;
  paletteDirty = false;
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
  ZoneScopedN("CanvasView.snapVisualToTargets");
  for (int y = 0; y < activeViewHeight; ++y) {
    for (int x = 0; x < activeViewWidth; ++x) {
      const int index = y * textureWidth + x;
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
  ZoneScopedN("CanvasView.tickVisual");
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
  for (int y = 0; y < activeViewHeight; ++y) {
    for (int x = 0; x < activeViewWidth; ++x) {
      const int index = y * textureWidth + x;
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
  if (!regionReady || x < 0 || y < 0 || x >= activeViewWidth ||
      y >= activeViewHeight) {
    return CellAddress{ std::numeric_limits<std::int64_t>::max(),
                        std::numeric_limits<std::int64_t>::max() };
  }
  const int sourceX = x * visibleCellWidth / activeViewWidth;
  const int sourceY = y * visibleCellHeight / activeViewHeight;
  return CellAddress{ visibleFirstCell.x + sourceX,
                      visibleFirstCell.y - sourceY };
}

void
CanvasView::DrawImpl()
{
}

bool
CanvasView::AppendCommands(Renderer* activeRenderer)
{
  ZoneScopedN("CanvasView.AppendCommands");
  if (!isVisible() || !gpuReady || activeRenderer == nullptr) {
    return isVisible();
  }
  if (textureUploadPending && activeViewWidth > 0 && activeViewHeight > 0) {
    ZoneScopedN("CanvasView.UpdateDisplayTexture");
    int x = std::clamp(uploadMinX, 0, activeViewWidth - 1);
    int y = std::clamp(uploadMinY, 0, activeViewHeight - 1);
    int width = uploadMaxX - x + 1;
    int height = uploadMaxY - y + 1;
    if (width <= 0 || height <= 0) {
      x = 0;
      y = 0;
      width = activeViewWidth;
      height = activeViewHeight;
    }
    activeRenderer->pushUpdateTexture(
      displayTextureHandle,
      x,
      y,
      std::min(width, activeViewWidth - x),
      std::min(height, activeViewHeight - y),
      3,
      texBuffer +
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(textureWidth) +
         static_cast<std::size_t>(x)) *
          3u,
      textureWidth);
    textureUploadPending = false;
    resetUploadBounds();
  }
  visual.setRenderer(activeRenderer);
  visual.setVisible(isVisible());
  return visual.AppendCommands(activeRenderer);
}
