#pragma once
#include <Illumo/Rendering/GLString.h>
#include <Illumo/Rendering/Primitives/UiTheme.h>
#include <chrono>

class SplashText : public GLString
{
public:
  SplashText()
    : startTime(std::chrono::high_resolution_clock::now())
  {
    setPanelStyle(UiTheme::noticePanel(ColorRgba{ 255, 230, 120, 255 }));
    setVisible(false);
  }

  // Corner mode-label splash (EDIT / NORMAL, etc.). Starts hidden until Wake().
  SplashText(std::string content,
             int r,
             int g,
             int b,
             int a,
             int size_pt,
             int x,
             int y,
             Renderer* renderer = nullptr)
    : GLString(content, r, g, b, a, size_pt, x, y, renderer)
    , startTime(std::chrono::high_resolution_clock::now())
    , wakeDuration(std::chrono::duration<float>(1.5f))
    , fadeRate(1.0f)
  {
    setPanelStyle(
      UiTheme::noticePanel(ColorRgba{ static_cast<unsigned char>(r),
                                      static_cast<unsigned char>(g),
                                      static_cast<unsigned char>(b),
                                      static_cast<unsigned char>(a) }));
    setVisible(false);
  }

  ~SplashText() = default;

  void DrawImpl()
  {
    GLString::DrawImpl();
    Fade();
  }

  bool AppendCommands(Renderer* rend) override
  {
    // Only emit tokens while awake; still tick Fade so opacity drops.
    bool migrated = true;
    if (isVisible()) {
      migrated = GLString::AppendCommands(rend);
    }
    Fade();
    return migrated;
  }

  void Wake();
  void Fade();

private:
  std::chrono::high_resolution_clock::time_point startTime;
  std::chrono::duration<float> wakeDuration =
    std::chrono::duration<float>(1.5f);
  float fadeRate = 1.0f;
};
