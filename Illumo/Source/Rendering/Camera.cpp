#include <Illumo/Rendering/Camera.h>
#include <Illumo/Rendering/IRenderWindow.h>
#include <algorithm>

Camera::Camera(const glm::vec2& initialPos,
               float initialZoom,
               IEnvVars* envVars)
  : position(initialPos)
  , targetPosition(initialPos)
  , rotation(0.0f)
  , targetRotation(0.0f)
  , zoom(initialZoom)
  , targetZoom(initialZoom)
  , envVars(envVars)
  , projectionType(ProjectonType::ORTOGRAPHIC)
  , smoothingSpeed(15.0f) // default interpolation speed
{
}

void
Camera::Update(float deltaTime)
{
  if (deltaTime <= 0.0f)
    return;

  // Prevent large deltas from blowing up the interpolation
  float t = std::min(1.0f, deltaTime * smoothingSpeed);

  position += (targetPosition - position) * static_cast<double>(t);
  zoom = glm::mix(zoom, targetZoom, t);
  rotation = glm::mix(rotation, targetRotation, t);
}

void
Camera::Pan(const glm::vec2& offset)
{
  Pan(glm::dvec2(static_cast<double>(offset.x), static_cast<double>(offset.y)));
}

void
Camera::Pan(const glm::dvec2& offset)
{
  // Panning offset in pixels is proportional to zoom level (more zoom = slower
  // panning)
  targetPosition += offset / static_cast<double>(targetZoom);
}

void
Camera::Rotate(float angle)
{
  targetRotation += angle;
}

void
Camera::ZoomAt(float zoomFactor, const glm::vec2& zoomCenter)
{
  ZoomAt(zoomFactor,
         glm::dvec2(static_cast<double>(zoomCenter.x),
                    static_cast<double>(zoomCenter.y)));
}

void
Camera::ZoomAt(float zoomFactor, const glm::dvec2& zoomCenter)
{
  float oldTargetZoom = targetZoom;
  targetZoom = std::clamp(targetZoom * zoomFactor, 0.1f, 100.0f);

  // Zoom center is in pixels (screen space)
  targetPosition =
    zoomCenter - (zoomCenter - targetPosition) *
                   static_cast<double>(oldTargetZoom / targetZoom);
}

void
Camera::Reset()
{
  targetPosition = glm::dvec2(0.0, 0.0);
  position = targetPosition;
  targetZoom = 1.0f;
  zoom = targetZoom;
}

std::array<int, 2>
Camera::GetWinDims() const
{
  int winX = 1280;
  int winY = 720;
  if (envVars) {
    long vx = envVars->getVar("WinX").valueAsLong;
    long vy = envVars->getVar("WinY").valueAsLong;
    if (vx > 0) {
      winX = static_cast<int>(vx);
    }
    if (vy > 0) {
      winY = static_cast<int>(vy);
    }
  }
  return { winX, winY };
}

glm::mat4
Camera::GetViewMatrix() const
{
  std::array<int, 2> winDims = GetWinDims();
  float halfW = winDims[0] / 2.0f;
  float halfH = winDims[1] / 2.0f;

  glm::mat4 view = glm::mat4(1.0f);
  // 1. Move origin to screen center for rotation/scaling
  view = glm::translate(view, glm::vec3(halfW, halfH, 0.0f));
  view =
    glm::rotate(view, glm::radians(-rotation), glm::vec3(0.0f, 0.0f, 1.0f));
  // 2. Scale
  view = glm::scale(view, glm::vec3(zoom, zoom, 1.0f));
  // 3. Move relative to camera target position
  view = glm::translate(view,
                        glm::vec3(static_cast<float>(-position.x),
                                  static_cast<float>(-position.y),
                                  0.0f));
  // 4. Rotate
  return view;
}

glm::mat4
Camera::GetProjectionMatrix(float /*aspectRatio*/) const
{
  std::array<int, 2> winDims = GetWinDims();

  switch (projectionType) {
    case ProjectonType::ORTOGRAPHIC:
      // Orthographic projection mapping screen pixel space to NDC [-1, 1], with
      // y going up (0 at bottom, h at top)
      return glm::ortho(
        0.0f, (float)winDims[0], 0.0f, (float)winDims[1], -1.0f, 1.0f);
    case ProjectonType::PERSPECTIVE:
      // Perspective projection mapping screen pixel space to NDC [-1, 1]
      return glm::perspective(glm::radians(45.0f),
                              (float)winDims[0] / (float)winDims[1],
                              0.1f,
                              100.0f);
  }
  return glm::mat4(1.0f);
}

glm::mat4
Camera::GetMVPMatrix(float aspectRatio) const
{
  return GetProjectionMatrix(aspectRatio) * GetViewMatrix();
}

glm::vec2
Camera::ScreenToWorld(const glm::vec2& screenPos) const
{
  const glm::dvec2 world = ScreenToWorldPrecise(glm::dvec2(
    static_cast<double>(screenPos.x), static_cast<double>(screenPos.y)));
  return glm::vec2(static_cast<float>(world.x), static_cast<float>(world.y));
}

glm::dvec2
Camera::ScreenToWorldPrecise(const glm::dvec2& screenPos) const
{
  std::array<int, 2> winDims = GetWinDims();
  const double halfW = static_cast<double>(winDims[0]) / 2.0;
  const double halfH = static_cast<double>(winDims[1]) / 2.0;

  const double dx = screenPos.x - halfW;
  const double dy = halfH - screenPos.y;

  const double rad = glm::radians(static_cast<double>(rotation));
  const double cosRad = std::cos(rad);
  const double sinRad = std::sin(rad);

  const double rotatedX = dx * cosRad - dy * sinRad;
  const double rotatedY = dx * sinRad + dy * cosRad;

  const double worldX = rotatedX / static_cast<double>(zoom) + position.x;
  const double worldY = rotatedY / static_cast<double>(zoom) + position.y;

  return glm::dvec2(worldX, worldY);
}
