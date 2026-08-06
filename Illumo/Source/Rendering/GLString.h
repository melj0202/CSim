// Screen-space string drawn via stb_easy_font + token renderer.
#pragma once

#include "Drawable.h"
#include <cstdint>
#include <string>

struct VertexData
{
  float x, y, z;
  uint8_t color[4];
};

class IRenderWindow;
class Renderer;

class GLString : public Drawable<GLString>
{

private:
  std::string content;
  int r, g, b, a;
  float size_pt;
  int x, y;
  VertexData vertices[2000 * 4];
  unsigned int cachedNumQuads;
  bool geometryDirty;
  bool gpuUploadPending;

  Renderer* renderer;
  unsigned long meshHandle;
  unsigned long shaderHandle;
  bool gpuReady;

  void enrollGpuResources();
  void rebuildGeometry();
  void markGeometryDirty()
  {
    geometryDirty = true;
    gpuUploadPending = true;
  }

public:
  static inline IRenderWindow* s_window = nullptr;
  static void setRenderWindow(IRenderWindow* window) { s_window = window; }

  GLString();
  GLString(std::string content,
           int r,
           int g,
           int b,
           int a,
           int size_pt,
           int x,
           int y,
           Renderer* renderer = nullptr);
  ~GLString();

  void setRenderer(Renderer* r);
  void setContent(std::string newContent);
  void setR(int newR);
  void setG(int newG);
  void setB(int newB);
  void setA(int newA);
  void setSize(int newSize);
  void setX(int newX);
  void setY(int newY);
  void DrawImpl();
  bool AppendCommands(Renderer* renderer) override;

  std::string getContent();
  int getR();
  int getG();
  int getB();
  int getA();
  int getSize();
  int getX();
  int getY();
};
