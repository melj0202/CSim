#pragma once

/*
        This class defines a cell brush that paints cells onto the canvas.
*/
class Brush
{
private:
  Canvas* target;

public:
  void Paint(unsigned char colorVal)
  {
    static int lastMouseX = -1;
    static int lastMouseY = -1;
    static bool wasPressed = false;
    int width = target->canvasWidth;
    int height = target->canvasHeight;

    if (wasPressed) {
      int x0 = lastMouseX;
      int y0 = lastMouseY;
      int x1 = currentX;
      int y1 = currentY;
      int dx = abs(x1 - x0);
      int dy = abs(y1 - y0);
      int sx = (x0 < x1) ? 1 : -1;
      int sy = (y0 < y1) ? 1 : -1;
      int err = dx - dy;

      while (true) {
        if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
          this->target->setCanvasPixel(x0, y0, colorVal);
        }
        if (x0 == x1 && y0 == y1)
          break;
        int e2 = 2 * err;
        if (e2 > -dy) {
          err -= dy;
          x0 += sx;
        }
        if (e2 < dx) {
          err += dx;
          y0 += sy;
        }
      }
    } else {
      if (currentX >= 0 && currentX < width && currentY >= 0 &&
          currentY < height) {
        this->target->setCanvasPixel(currentX, currentY, colorVal);
      }
    }
    wasPressed = true;
    lastMouseX = currentX;
    lastMouseY = currentY;
  }
};