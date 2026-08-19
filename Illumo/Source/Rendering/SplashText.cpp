#include <Illumo/Rendering/SplashText.h>

void
SplashText::Wake()
{
  if (getContent() == "EDIT") {
    const ColorRgba color = UiTheme::warning();
    setR(color.r);
    setG(color.g);
    setB(color.b);
    setPanelStyle(UiTheme::noticePanel(color));
  } else if (getContent() == "NORMAL") {
    const ColorRgba color = UiTheme::accent();
    setR(color.r);
    setG(color.g);
    setB(color.b);
    setPanelStyle(UiTheme::noticePanel(color));
  }
  setVisible(true);
  setA(255);
  startTime = std::chrono::high_resolution_clock::now();
}

void
SplashText::Fade()
{
  if (!isVisible()) {
    return;
  }

  // Linear fade from opaque to transparent over wakeDuration, then hide.
  std::chrono::high_resolution_clock::time_point now =
    std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> durationElapsed = now - startTime;
  const float total = wakeDuration.count();
  if (total <= 0.0f || durationElapsed.count() >= total) {
    setA(0);
    setVisible(false);
    return;
  }

  float t = durationElapsed.count() / total;
  if (t < 0.0f) {
    t = 0.0f;
  }
  if (t > 1.0f) {
    t = 1.0f;
  }
  const float opacity = 255.0f * (1.0f - t);
  setA(static_cast<int>(opacity + 0.5f));
  if (opacity <= 1.0f) {
    setA(0);
    setVisible(false);
  }
}
