#pragma once

#include "Rendering/Drawable.h"
#include "Rendering/IRenderWindow.h"
#include "Rendering/Primitives/ShapePrimitive.h"
#include "Rendering/Primitives/SpritePrimitive.h"
#include "Rendering/Primitives/TextPrimitive.h"
#include "Rendering/RenderLayerId.h"
#include <cstddef>
#include <string>
#include <vector>

class Camera;
class Renderer;

// Drawable host for composed render primitives (D-R12).
// Game/editor objects include a GameVisual and add shapes/sprites/text; they
// do not hand-roll enroll/bind/draw tokens. Higher-level classes embed
// GameVisual to build complex visuals from simple primitives.
//
// Scene still lists this host once per layer — not each primitive.
class GameVisual : public DrawableBase
{
public:
  // Console help pages and multi-line UI need a large batch; stack-friendly
  // consumers should keep GameVisual on the heap or as a long-lived member.
  static const unsigned int kMaxQuads = 8192;

  GameVisual();
  ~GameVisual() override;

  void setRenderer(Renderer* renderer);
  void setWindow(IRenderWindow* window);
  void setCamera(Camera* camera);
  void setSpace(PrimitiveSpace space);
  PrimitiveSpace getSpace() const { return space; }

  void setLayerHint(RenderLayerId layer) { layerHint = layer; }
  RenderLayerId getLayerHint() const { return layerHint; }

  void prepare(Renderer* renderer);

  void clearPrimitives();
  size_t shapeCount() const { return shapes.size(); }
  size_t spriteCount() const { return sprites.size(); }
  size_t textCount() const { return texts.size(); }

  size_t addFilledRect(float x, float y, float w, float h, ColorRgba color);
  size_t addOutlineRect(float x,
                        float y,
                        float w,
                        float h,
                        ColorRgba color,
                        float lineWidth = 1.0f);
  size_t addLine(float x0,
                 float y0,
                 float x1,
                 float y1,
                 ColorRgba color,
                 float lineWidth = 1.0f);
  size_t addSprite(unsigned long textureHandle,
                   float x,
                   float y,
                   float w,
                   float h,
                   ColorRgba tint = ColorRgba{},
                   float u0 = 0.0f,
                   float v0 = 0.0f,
                   float u1 = 1.0f,
                   float v1 = 1.0f);
  size_t addText(const std::string& content,
                 float x,
                 float y,
                 float sizePt,
                 ColorRgba color);

  ShapePrimitive* getShape(size_t index);
  SpritePrimitive* getSprite(size_t index);
  TextPrimitive* getText(size_t index);

  void Draw() override {}
  bool AppendCommands(Renderer* renderer) override;

private:
  struct ShapeVertex
  {
    float x, y, z;
    unsigned char r, g, b, a;
  };

  struct SpriteVertex
  {
    float x, y, z;
    unsigned char r, g, b, a;
    float u, v;
  };

  Renderer* renderer = nullptr;
  IRenderWindow* window = nullptr;
  Camera* camera = nullptr;
  PrimitiveSpace space = PrimitiveSpace::Pixels;
  RenderLayerId layerHint = RenderLayerId::World;

  std::vector<ShapePrimitive> shapes;
  std::vector<SpritePrimitive> sprites;
  std::vector<TextPrimitive> texts;

  unsigned long shapeMeshHandle = 0;
  unsigned long spriteMeshHandle = 0;
  bool gpuReady = false;
  bool geometryDirty = true;
  bool shapeUploadPending = false;
  bool spriteUploadPending = false;

  std::vector<ShapeVertex> shapeVerts;
  std::vector<SpriteVertex> spriteVerts;
  unsigned int shapeQuadCount = 0;

  struct SpriteBatch
  {
    unsigned long textureHandle = 0;
    unsigned int firstQuad = 0;
    unsigned int quadCount = 0;
  };
  SpriteBatch spriteBatches[128];
  unsigned int spriteBatchCount = 0;
  unsigned int spriteQuadCount = 0;

  void markDirty() { geometryDirty = true; }
  void enrollGpuResources();
  void rebuildGeometry();
  bool pushShapeQuad(float x0, float y0, float x1, float y1, ColorRgba color);
  bool pushLineAsQuad(float x0,
                      float y0,
                      float x1,
                      float y1,
                      float width,
                      ColorRgba color);
  bool pushSpriteQuad(const SpritePrimitive& sprite);
  bool pushTextRun(const TextPrimitive& text);
};
