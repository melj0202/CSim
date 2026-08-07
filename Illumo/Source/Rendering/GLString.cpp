#include "GLString.h"
#include "IRenderWindow.h"
#include "Logger.h"
#include "Renderer.h"

GLString::GLString()
  : content("")
  , r(255)
  , g(255)
  , b(255)
  , a(255)
  , size_pt(12.0f)
  , x(0)
  , y(0)
  , contentDirty(true)
  , renderer(nullptr)
{
  visual.setSpace(PrimitiveSpace::Pixels);
  visual.setLayerHint(RenderLayerId::UI);
}

GLString::GLString(std::string content,
                   int r,
                   int g,
                   int b,
                   int a,
                   int size_pt,
                   int x,
                   int y,
                   Renderer* renderer)
  : content(content)
  , r(r)
  , g(g)
  , b(b)
  , a(a)
  , size_pt(static_cast<float>(size_pt))
  , x(x)
  , y(y)
  , contentDirty(true)
  , renderer(renderer)
{
  visual.setSpace(PrimitiveSpace::Pixels);
  visual.setLayerHint(RenderLayerId::UI);
  if (renderer) {
    visual.setRenderer(renderer);
    visual.prepare(renderer);
  }
  if (s_window) {
    visual.setWindow(s_window);
  }
  syncVisual();
}

GLString::~GLString() = default;

void
GLString::setRenderer(Renderer* rend)
{
  renderer = rend;
  visual.setRenderer(rend);
  if (renderer) {
    visual.prepare(renderer);
  }
  markContentDirty();
}

void
GLString::syncVisual()
{
  visual.clearPrimitives();
  if (!content.empty()) {
    ColorRgba color{ static_cast<unsigned char>(r),
                     static_cast<unsigned char>(g),
                     static_cast<unsigned char>(b),
                     static_cast<unsigned char>(a) };
    visual.addText(
      content, static_cast<float>(x), static_cast<float>(y), size_pt, color);
  }
  contentDirty = false;
}

void
GLString::setContent(std::string newContent)
{
  if (content != newContent) {
    content = newContent;
    markContentDirty();
  }
}

void
GLString::setR(int newR)
{
  if (r != newR) {
    r = newR;
    markContentDirty();
  }
}
void
GLString::setG(int newG)
{
  if (g != newG) {
    g = newG;
    markContentDirty();
  }
}
void
GLString::setB(int newB)
{
  if (b != newB) {
    b = newB;
    markContentDirty();
  }
}
void
GLString::setA(int newA)
{
  if (a != newA) {
    a = newA;
    markContentDirty();
  }
}
void
GLString::setSize(int newSize)
{
  float s = static_cast<float>(newSize);
  if (size_pt != s) {
    size_pt = s;
    markContentDirty();
  }
}
void
GLString::setX(int newX)
{
  if (x != newX) {
    x = newX;
    markContentDirty();
  }
}
void
GLString::setY(int newY)
{
  if (y != newY) {
    y = newY;
    markContentDirty();
  }
}

std::string
GLString::getContent()
{
  return content;
}
int
GLString::getR()
{
  return r;
}
int
GLString::getG()
{
  return g;
}
int
GLString::getB()
{
  return b;
}
int
GLString::getA()
{
  return a;
}
int
GLString::getSize()
{
  return static_cast<int>(size_pt);
}
int
GLString::getX()
{
  return x;
}
int
GLString::getY()
{
  return y;
}

void
GLString::DrawImpl()
{
}

bool
GLString::AppendCommands(Renderer* rend)
{
  if (!isVisible()) {
    return true;
  }
  if (content.empty()) {
    return true;
  }
  if (!rend) {
    return false;
  }
  if (!s_window) {
    return true;
  }

  if (contentDirty || renderer != rend) {
    renderer = rend;
    syncVisual();
  }

  visual.setRenderer(rend);
  visual.setWindow(s_window);
  visual.setSpace(PrimitiveSpace::Pixels);
  visual.setVisible(true);
  return visual.AppendCommands(rend);
}
