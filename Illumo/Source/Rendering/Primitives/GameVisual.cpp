#include "GameVisual.h"
#include "Rendering/Camera.h"
#include "Rendering/IMesh.h"
#include "Rendering/Renderer.h"
#include "Services/Logger.h"
#include "thirdparty/stb/stb_easy_font.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <glm/glm.hpp>
#include <vector>

GameVisual::GameVisual()
{
  shapeVerts.resize(static_cast<size_t>(kMaxQuads) * 4);
  spriteVerts.resize(static_cast<size_t>(kMaxQuads) * 4);
}

GameVisual::~GameVisual()
{
  gpuReady = false;
}

void
GameVisual::setRenderer(Renderer* r)
{
  renderer = r;
  if (!gpuReady && renderer) {
    enrollGpuResources();
  }
}

void
GameVisual::setWindow(IRenderWindow* w)
{
  window = w;
}

void
GameVisual::setCamera(Camera* cam)
{
  camera = cam;
}

void
GameVisual::setSpace(PrimitiveSpace s)
{
  if (space != s) {
    space = s;
    markDirty();
  }
}

void
GameVisual::prepare(Renderer* r)
{
  if (r) {
    renderer = r;
  }
  enrollGpuResources();
}

void
GameVisual::clearPrimitives()
{
  shapes.clear();
  sprites.clear();
  texts.clear();
  markDirty();
}

size_t
GameVisual::addFilledRect(float x, float y, float w, float h, ColorRgba color)
{
  ShapePrimitive shape;
  shape.kind = ShapeKind::FilledRect;
  shape.rect.x = x;
  shape.rect.y = y;
  shape.rect.w = w;
  shape.rect.h = h;
  shape.color = color;
  shapes.push_back(shape);
  markDirty();
  return shapes.size() - 1;
}

size_t
GameVisual::addOutlineRect(float x,
                           float y,
                           float w,
                           float h,
                           ColorRgba color,
                           float lineWidth)
{
  ShapePrimitive shape;
  shape.kind = ShapeKind::OutlineRect;
  shape.rect.x = x;
  shape.rect.y = y;
  shape.rect.w = w;
  shape.rect.h = h;
  shape.color = color;
  shape.lineWidth = lineWidth;
  shapes.push_back(shape);
  markDirty();
  return shapes.size() - 1;
}

size_t
GameVisual::addLine(float x0,
                    float y0,
                    float x1,
                    float y1,
                    ColorRgba color,
                    float lineWidth)
{
  ShapePrimitive shape;
  shape.kind = ShapeKind::Line;
  shape.x0 = x0;
  shape.y0 = y0;
  shape.x1 = x1;
  shape.y1 = y1;
  shape.color = color;
  shape.lineWidth = lineWidth;
  shapes.push_back(shape);
  markDirty();
  return shapes.size() - 1;
}

size_t
GameVisual::addSprite(unsigned long textureHandle,
                      float x,
                      float y,
                      float w,
                      float h,
                      ColorRgba tint,
                      float u0,
                      float v0,
                      float u1,
                      float v1)
{
  SpritePrimitive sprite;
  sprite.textureHandle = textureHandle;
  sprite.rect.x = x;
  sprite.rect.y = y;
  sprite.rect.w = w;
  sprite.rect.h = h;
  sprite.tint = tint;
  sprite.u0 = u0;
  sprite.v0 = v0;
  sprite.u1 = u1;
  sprite.v1 = v1;
  sprites.push_back(sprite);
  markDirty();
  return sprites.size() - 1;
}

size_t
GameVisual::addText(const std::string& content,
                    float x,
                    float y,
                    float sizePt,
                    ColorRgba color)
{
  TextPrimitive text;
  text.content = content;
  text.x = x;
  text.y = y;
  text.sizePt = sizePt;
  text.color = color;
  texts.push_back(text);
  markDirty();
  return texts.size() - 1;
}

ShapePrimitive*
GameVisual::getShape(size_t index)
{
  if (index >= shapes.size()) {
    return nullptr;
  }
  markDirty();
  return &shapes[index];
}

SpritePrimitive*
GameVisual::getSprite(size_t index)
{
  if (index >= sprites.size()) {
    return nullptr;
  }
  markDirty();
  return &sprites[index];
}

TextPrimitive*
GameVisual::getText(size_t index)
{
  if (index >= texts.size()) {
    return nullptr;
  }
  markDirty();
  return &texts[index];
}

void
GameVisual::enrollGpuResources()
{
  if (!renderer) {
    return;
  }
  if (gpuReady && shapeMeshHandle != 0 && spriteMeshHandle != 0) {
    return;
  }

  renderer->ensureBuiltinStyles();

  std::vector<unsigned int> indices(static_cast<size_t>(kMaxQuads * 6));
  for (unsigned int i = 0; i < kMaxQuads; ++i) {
    indices[static_cast<size_t>(i * 6 + 0)] = i * 4 + 0;
    indices[static_cast<size_t>(i * 6 + 1)] = i * 4 + 1;
    indices[static_cast<size_t>(i * 6 + 2)] = i * 4 + 2;
    indices[static_cast<size_t>(i * 6 + 3)] = i * 4 + 2;
    indices[static_cast<size_t>(i * 6 + 4)] = i * 4 + 3;
    indices[static_cast<size_t>(i * 6 + 5)] = i * 4 + 0;
  }

  const size_t shapeVboBytes =
    static_cast<size_t>(kMaxQuads) * 4 * sizeof(ShapeVertex);
  shapeMeshHandle = renderer->allocateHandle();
  renderer->enrollDynamicMesh(shapeVboBytes,
                              indices.data(),
                              indices.size() * sizeof(unsigned int),
                              shapeMeshHandle,
                              MeshVertexLayout::Pos3Color4U8);

  const size_t spriteVboBytes =
    static_cast<size_t>(kMaxQuads) * 4 * sizeof(SpriteVertex);
  spriteMeshHandle = renderer->allocateHandle();
  renderer->enrollDynamicMesh(spriteVboBytes,
                              indices.data(),
                              indices.size() * sizeof(unsigned int),
                              spriteMeshHandle,
                              MeshVertexLayout::Pos3Color4U8Uv2);

  gpuReady = true;
  geometryDirty = true;
  Logger::LogTrace("GameVisual enrolled (shape + sprite meshes)");
}

bool
GameVisual::pushShapeQuad(float x0,
                          float y0,
                          float x1,
                          float y1,
                          ColorRgba color)
{
  if (shapeQuadCount >= kMaxQuads) {
    return false;
  }
  const unsigned int base = shapeQuadCount * 4;
  shapeVerts[base + 0] = { x0, y0, 0.0f, color.r, color.g, color.b, color.a };
  shapeVerts[base + 1] = { x1, y0, 0.0f, color.r, color.g, color.b, color.a };
  shapeVerts[base + 2] = { x1, y1, 0.0f, color.r, color.g, color.b, color.a };
  shapeVerts[base + 3] = { x0, y1, 0.0f, color.r, color.g, color.b, color.a };
  shapeQuadCount += 1;
  return true;
}

bool
GameVisual::pushLineAsQuad(float x0,
                           float y0,
                           float x1,
                           float y1,
                           float width,
                           ColorRgba color)
{
  float dx = x1 - x0;
  float dy = y1 - y0;
  float len = std::sqrt(dx * dx + dy * dy);
  if (len < 1.0e-6f) {
    return pushShapeQuad(x0 - width * 0.5f,
                         y0 - width * 0.5f,
                         x0 + width * 0.5f,
                         y0 + width * 0.5f,
                         color);
  }
  float nx = (-dy / len) * (width * 0.5f);
  float ny = (dx / len) * (width * 0.5f);

  if (shapeQuadCount >= kMaxQuads) {
    return false;
  }
  const unsigned int base = shapeQuadCount * 4;
  shapeVerts[base + 0] = { x0 + nx, y0 + ny, 0.0f,   color.r,
                           color.g, color.b, color.a };
  shapeVerts[base + 1] = { x1 + nx, y1 + ny, 0.0f,   color.r,
                           color.g, color.b, color.a };
  shapeVerts[base + 2] = { x1 - nx, y1 - ny, 0.0f,   color.r,
                           color.g, color.b, color.a };
  shapeVerts[base + 3] = { x0 - nx, y0 - ny, 0.0f,   color.r,
                           color.g, color.b, color.a };
  shapeQuadCount += 1;
  return true;
}

bool
GameVisual::pushSpriteQuad(const SpritePrimitive& sprite)
{
  if (spriteQuadCount >= kMaxQuads) {
    return false;
  }
  const float x0 = sprite.rect.x;
  const float y0 = sprite.rect.y;
  const float x1 = sprite.rect.x + sprite.rect.w;
  const float y1 = sprite.rect.y + sprite.rect.h;
  const unsigned int base = spriteQuadCount * 4;
  const ColorRgba c = sprite.tint;
  spriteVerts[base + 0] = { x0,  y0,  0.0f,      c.r,      c.g,
                            c.b, c.a, sprite.u0, sprite.v0 };
  spriteVerts[base + 1] = { x1,  y0,  0.0f,      c.r,      c.g,
                            c.b, c.a, sprite.u1, sprite.v0 };
  spriteVerts[base + 2] = { x1,  y1,  0.0f,      c.r,      c.g,
                            c.b, c.a, sprite.u1, sprite.v1 };
  spriteVerts[base + 3] = { x0,  y1,  0.0f,      c.r,      c.g,
                            c.b, c.a, sprite.u0, sprite.v1 };
  spriteQuadCount += 1;
  return true;
}

bool
GameVisual::pushTextRun(const TextPrimitive& text)
{
  if (text.content.empty()) {
    return true;
  }

  // stb_easy_font writes VertexData-compatible verts (pos3 + color4).
  // Cap temp buffer to remaining shape capacity.
  const unsigned int remainingQuads = kMaxQuads - shapeQuadCount;
  if (remainingQuads == 0) {
    return false;
  }
  const size_t tempBytes =
    static_cast<size_t>(remainingQuads) * 4 * sizeof(ShapeVertex);
  std::vector<unsigned char> temp(tempBytes);

  unsigned char color[4] = {
    text.color.r, text.color.g, text.color.b, text.color.a
  };
  std::string mutableCopy = text.content;
  int numQuads = stb_easy_font_print(0.0f,
                                     0.0f,
                                     mutableCopy.data(),
                                     color,
                                     temp.data(),
                                     static_cast<int>(temp.size()));
  if (numQuads < 0) {
    numQuads = 0;
  }
  if (static_cast<unsigned int>(numQuads) > remainingQuads) {
    numQuads = static_cast<int>(remainingQuads);
  }

  const float scale = text.sizePt / 12.0f;
  const ShapeVertex* src = reinterpret_cast<const ShapeVertex*>(temp.data());
  for (int q = 0; q < numQuads; ++q) {
    const unsigned int base = shapeQuadCount * 4;
    for (int v = 0; v < 4; ++v) {
      const ShapeVertex& s = src[q * 4 + v];
      shapeVerts[base + static_cast<unsigned int>(v)].x = s.x * scale + text.x;
      shapeVerts[base + static_cast<unsigned int>(v)].y = s.y * scale + text.y;
      shapeVerts[base + static_cast<unsigned int>(v)].z = 0.0f;
      shapeVerts[base + static_cast<unsigned int>(v)].r = s.r;
      shapeVerts[base + static_cast<unsigned int>(v)].g = s.g;
      shapeVerts[base + static_cast<unsigned int>(v)].b = s.b;
      shapeVerts[base + static_cast<unsigned int>(v)].a = s.a;
    }
    shapeQuadCount += 1;
  }
  return true;
}

void
GameVisual::rebuildGeometry()
{
  shapeQuadCount = 0;
  spriteQuadCount = 0;
  spriteBatchCount = 0;

  for (size_t i = 0; i < shapes.size(); ++i) {
    const ShapePrimitive& shape = shapes[i];
    if (!shape.visible) {
      continue;
    }
    if (shape.kind == ShapeKind::FilledRect) {
      const float x0 = shape.rect.x;
      const float y0 = shape.rect.y;
      const float x1 = shape.rect.x + shape.rect.w;
      const float y1 = shape.rect.y + shape.rect.h;
      if (!pushShapeQuad(x0, y0, x1, y1, shape.color)) {
        Logger::LogWarning("GameVisual: shape quad capacity reached");
        break;
      }
    } else if (shape.kind == ShapeKind::OutlineRect) {
      const float x = shape.rect.x;
      const float y = shape.rect.y;
      const float w = shape.rect.w;
      const float h = shape.rect.h;
      const float t = shape.lineWidth > 0.0f ? shape.lineWidth : 1.0f;
      bool ok = pushShapeQuad(x, y, x + w, y + t, shape.color);
      ok = ok && pushShapeQuad(x, y + h - t, x + w, y + h, shape.color);
      ok = ok && pushShapeQuad(x, y + t, x + t, y + h - t, shape.color);
      ok = ok && pushShapeQuad(x + w - t, y + t, x + w, y + h - t, shape.color);
      if (!ok) {
        Logger::LogWarning("GameVisual: shape quad capacity reached");
        break;
      }
    } else if (shape.kind == ShapeKind::Line) {
      if (!pushLineAsQuad(shape.x0,
                          shape.y0,
                          shape.x1,
                          shape.y1,
                          shape.lineWidth > 0.0f ? shape.lineWidth : 1.0f,
                          shape.color)) {
        Logger::LogWarning("GameVisual: shape quad capacity reached");
        break;
      }
    }
  }

  for (size_t i = 0; i < texts.size(); ++i) {
    const TextPrimitive& text = texts[i];
    if (!text.visible) {
      continue;
    }
    if (!pushTextRun(text)) {
      Logger::LogWarning("GameVisual: text overflowed shape capacity");
      break;
    }
  }

  std::vector<size_t> order;
  order.reserve(sprites.size());
  for (size_t i = 0; i < sprites.size(); ++i) {
    if (sprites[i].visible && sprites[i].textureHandle != 0) {
      order.push_back(i);
    }
  }
  std::sort(order.begin(), order.end(), [this](size_t a, size_t b) {
    return sprites[a].textureHandle < sprites[b].textureHandle;
  });

  for (size_t oi = 0; oi < order.size(); ++oi) {
    const SpritePrimitive& sprite = sprites[order[oi]];
    const unsigned int quadBefore = spriteQuadCount;
    if (!pushSpriteQuad(sprite)) {
      Logger::LogWarning("GameVisual: sprite quad capacity reached");
      break;
    }
    if (spriteBatchCount == 0 ||
        spriteBatches[spriteBatchCount - 1].textureHandle !=
          sprite.textureHandle) {
      if (spriteBatchCount >= 128) {
        Logger::LogWarning("GameVisual: sprite batch limit reached");
        break;
      }
      spriteBatches[spriteBatchCount].textureHandle = sprite.textureHandle;
      spriteBatches[spriteBatchCount].firstQuad = quadBefore;
      spriteBatches[spriteBatchCount].quadCount = 1;
      spriteBatchCount += 1;
    } else {
      spriteBatches[spriteBatchCount - 1].quadCount += 1;
    }
  }

  shapeUploadPending = (shapeQuadCount > 0);
  spriteUploadPending = (spriteQuadCount > 0);
  geometryDirty = false;
}

bool
GameVisual::AppendCommands(Renderer* r)
{
  if (!isVisible()) {
    return true;
  }
  if (!r) {
    return false;
  }
  if (!gpuReady) {
    renderer = r;
    enrollGpuResources();
  }
  if (!gpuReady) {
    return false;
  }

  if (geometryDirty) {
    rebuildGeometry();
  }

  if (shapeQuadCount == 0 && spriteQuadCount == 0) {
    return true;
  }

  float width = 1280.0f;
  float height = 720.0f;
  if (window) {
    std::array<int, 2> dims = window->getWindowDimensions();
    width = static_cast<float>(dims[0]);
    height = static_cast<float>(dims[1]);
  }

  const int usePixels = (space == PrimitiveSpace::Pixels) ? 1 : 0;
  float mvp[16] = {
    1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
  };
  if (space == PrimitiveSpace::World && camera && window) {
    std::array<int, 2> dims = window->getWindowDimensions();
    float aspect = static_cast<float>(dims[0]) /
                   static_cast<float>(dims[1] > 0 ? dims[1] : 1);
    glm::mat4 matrix = camera->GetMVPMatrix(aspect);
    std::memcpy(mvp, &matrix[0][0], 16 * sizeof(float));
  }

  if (shapeQuadCount > 0) {
    if (!r->bindStyle(RenderStyleId::Shape)) {
      return false;
    }
    r->pushSetMesh(shapeMeshHandle);
    r->pushUniformInt("uUsePixels", usePixels);
    r->pushUniformVec2("u_resolution", width, height);
    r->pushUniformMat4("uMVP", mvp);
    if (shapeUploadPending) {
      r->pushUpdateBuffer(
        shapeMeshHandle,
        0,
        static_cast<unsigned int>(shapeQuadCount * 4 * sizeof(ShapeVertex)),
        shapeVerts.data());
      shapeUploadPending = false;
    }
    r->pushDrawIndexed(shapeQuadCount * 6, 0);
  }

  if (spriteQuadCount > 0) {
    if (!r->bindStyle(RenderStyleId::Sprite)) {
      return false;
    }
    r->pushSetMesh(spriteMeshHandle);
    r->pushUniformInt("uUsePixels", usePixels);
    r->pushUniformVec2("u_resolution", width, height);
    r->pushUniformMat4("uMVP", mvp);
    r->pushUniformInt("uTexture", 0);

    if (spriteUploadPending) {
      r->pushUpdateBuffer(
        spriteMeshHandle,
        0,
        static_cast<unsigned int>(spriteQuadCount * 4 * sizeof(SpriteVertex)),
        spriteVerts.data());
      spriteUploadPending = false;
    }

    for (unsigned int b = 0; b < spriteBatchCount; ++b) {
      const SpriteBatch& batch = spriteBatches[b];
      r->pushSetTexture(batch.textureHandle, 0);
      r->pushDrawIndexed(batch.quadCount * 6, batch.firstQuad * 6);
    }
  }

  return true;
}
