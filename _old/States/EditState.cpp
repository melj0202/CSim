#include "Canvas.h"
#include "IRenderWindow.h"
#include "ServiceLocator.h"
#include "Core/CellContext.h"
#include "CommandLine.h"
#include "Rendering/Camera.h"

State* EditState::iterate(RuleSet* ruleSet, const char* filename, State* prevState)
{
	CellContext* cellContext = ServiceLocator::get<CellContext>();
	int width = cellContext->getCellCanvas()->canvasWidth;
	int height = cellContext->getCellCanvas()->canvasHeight;
	IRenderWindow* window = ServiceLocator::get<IRenderWindow>();

	// Replace the old mouse scaling code:
	std::array<double, 2> mouseCoords = window->getMouseCoords();
	std::array<int, 2> winDims = window->getWindowDimensions();
	Camera* camera = ServiceLocator::get<Camera>();

	glm::vec2 worldPos = camera->ScreenToWorld(glm::vec2(mouseCoords[0], mouseCoords[1]));

	// Map from screen space back to grid cell coordinates [0, width/height] using world cellSize
	float cellSize = 16.0f;
	int currentX = static_cast<int>(std::floor(worldPos.x / cellSize));
	int currentY = static_cast<int>(std::floor(worldPos.y / cellSize));

	if (!ServiceLocator::get<CommandLine>()->isOpen) {

		
		bool isRightPressed = glfwGetMouseButton(window->getWindowInstance(), GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
		bool isLeftPressed = glfwGetMouseButton(window->getWindowInstance(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

		if (isRightPressed || isLeftPressed) {
			unsigned char colorVal = isRightPressed ? 1 : 0;

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
						cellContext->getCellCanvas()->setCanvasPixel(x0, y0, colorVal);
					}
					if (x0 == x1 && y0 == y1) break;
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
				if (currentX >= 0 && currentX < width && currentY >= 0 && currentY < height) {
					cellContext->getCellCanvas()->setCanvasPixel(currentX, currentY, colorVal);
				}
			}
			wasPressed = true;
			lastMouseX = currentX;
			lastMouseY = currentY;
		}
		else {
			wasPressed = false;
			if (glfwGetKey(window->getWindowInstance(), GLFW_KEY_C) == GLFW_PRESS)
				cellContext->getCellCanvas()->clearCanvas();
		}
	}
	else {
		wasPressed = false;
	}

	for (int i = 0; i < width * height; i++) {
		ruleSet->evalCell(cellContext->getCellCanvas()->lifeCanvas[i], &cellContext->getCellCanvas()->texCanvasBuffer[i * 3]);
	}
//	RenderWindow::updateWindow();
	return prevState;
}
