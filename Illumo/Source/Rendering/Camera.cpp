#include "Camera.h"
#include "IRenderWindow.h"
#include <algorithm>


Camera::Camera(const glm::vec2& initialPos, float initialZoom, IEnvVars* envVars)
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
	if (initialPos == glm::vec2(0.0f, 0.0f))
	{
		long canvasX = 80;
		long canvasY = 60;

		if (envVars)
		{
			canvasX = envVars->getVar("CanvasX").valueAsLong;
			canvasY = envVars->getVar("CanvasY").valueAsLong;
		}
		float cellSize = 16.0f;
		position = glm::vec2((canvasX * cellSize) / 2.0f, (canvasY * cellSize) / 2.0f);
		targetPosition = position;
	}
}

void Camera::Update(float deltaTime)
{
	if (deltaTime <= 0.0f) return;

	// Prevent large deltas from blowing up the interpolation
	float t = std::min(1.0f, deltaTime * smoothingSpeed);

	position = glm::mix(position, targetPosition, t);
	zoom = glm::mix(zoom, targetZoom, t);
	rotation = glm::mix(rotation, targetRotation, t);
}

void Camera::Pan(const glm::vec2& offset)
{
// Panning offset in pixels is proportional to zoom level (more zoom = slower panning)
	targetPosition += offset / targetZoom;
}

void Camera::Rotate(float angle)
{
	targetRotation += angle;
}

void Camera::ZoomAt(float zoomFactor, const glm::vec2& zoomCenter)
{
	float oldTargetZoom = targetZoom;
	targetZoom = std::clamp(targetZoom * zoomFactor, 0.1f, 100.0f);

	// Zoom center is in pixels (screen space)
	targetPosition = zoomCenter - (zoomCenter - targetPosition) * (oldTargetZoom / targetZoom);
}

void Camera::Reset()
{
	long canvasX = 80;
	long canvasY = 60;

	if (envVars)
	{
		canvasX = envVars->getVar("CanvasX").valueAsLong;
		canvasY = envVars->getVar("CanvasY").valueAsLong;
	}
	float cellSize = 16.0f;
	targetPosition = glm::vec2((canvasX * cellSize) / 2.0f, (canvasY * cellSize) / 2.0f);
	targetZoom = 1.0f;
}

glm::mat4 Camera::GetViewMatrix() const
{
	std::array<int, 2> winDims{
		static_cast<int>(envVars->getVar("WinX").valueAsLong),
		static_cast<int>(envVars->getVar("WinY").valueAsLong)
	};
	float halfW = winDims[0] / 2.0f;
	float halfH = winDims[1] / 2.0f;

	glm::mat4 view = glm::mat4(1.0f);
	// 1. Move origin to screen center for rotation/scaling
	view = glm::translate(view, glm::vec3(halfW, halfH, 0.0f));
	view = glm::rotate(view, glm::radians(-rotation), glm::vec3(0.0f, 0.0f, 1.0f));
	// 2. Scale
	view = glm::scale(view, glm::vec3(zoom, zoom, 1.0f));
	// 3. Move relative to camera target position
	view = glm::translate(view, glm::vec3(-position, 0.0f));
	//4. Rotate
	return view;
}

glm::mat4 Camera::GetProjectionMatrix(float /*aspectRatio*/) const
{
	std::array<int, 2> winDims{
		static_cast<int>(envVars->getVar("WinX").valueAsLong),
		static_cast<int>(envVars->getVar("WinY").valueAsLong)
	};

	switch (projectionType)
	{
		case ProjectonType::ORTOGRAPHIC:
			// Orthographic projection mapping screen pixel space to NDC [-1, 1], with y going up (0 at bottom, h at top)
			return glm::ortho(0.0f, (float) winDims[0], 0.0f, (float) winDims[1], -1.0f, 1.0f);
		case ProjectonType::PERSPECTIVE:
			// Perspective projection mapping screen pixel space to NDC [-1, 1]
			return glm::perspective(glm::radians(45.0f), (float) winDims[0] / (float) winDims[1], 0.1f, 100.0f);
	}
	return glm::mat4(1.0f);
}

glm::mat4 Camera::GetMVPMatrix(float aspectRatio) const
{
	return GetProjectionMatrix(aspectRatio) * GetViewMatrix();
}

glm::vec2 Camera::ScreenToWorld(const glm::vec2& screenPos) const
{
	std::array<int, 2> winDims{
		static_cast<int>(envVars->getVar("WinX").valueAsLong),
		static_cast<int>(envVars->getVar("WinY").valueAsLong)
	};
	float halfW = winDims[0] / 2.0f;
	float halfH = winDims[1] / 2.0f;

	float dx = screenPos.x - halfW;
	float dy = halfH - screenPos.y;

	float rad = glm::radians(rotation);
	float cosRad = std::cos(rad);
	float sinRad = std::sin(rad);

	float rotatedX = dx * cosRad - dy * sinRad;
	float rotatedY = dx * sinRad + dy * cosRad;

	float worldX = rotatedX / zoom + position.x;
	float worldY = rotatedY / zoom + position.y;

	return glm::vec2(worldX, worldY);
}
