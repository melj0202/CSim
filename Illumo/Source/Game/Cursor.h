#pragma once

#include "Rendering/Primitives/GameVisual.h"

// Editor/game cursor composed of shape primitives on a GameVisual host.
// Modules add getVisual() (or the Cursor if used as DrawableBase*) to Scene.
class Cursor : public GameVisual
{
public:
  Cursor();

  void init(Renderer* renderer, IRenderWindow* window, Camera* camera);
  void setCellSize(float size);
  void setColor(ColorRgba color);
  void setWorldPosition(float worldX, float worldY);
  void setFromCell(int cellX, int cellY);
  void rebuild();

private:
  float cellSize = 16.0f;
  float worldX = 0.0f;
  float worldY = 0.0f;
  ColorRgba color{ 80, 220, 255, 220 };
  bool initialized = false;
};
