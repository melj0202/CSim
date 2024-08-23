#include "CellEditState.h"

void CellEditState::iterate(CellCanvas& lifeCanvas, CellRuleSet& ruleSet, RenderWindow& window)
{
	std::array<double, 2> mouseCoords = window.getMouseCoords();

	std::array<double, 2> scaledMouseCoords = { mouseCoords[0] / (window.getWindowDimensions()[0] / lifeCanvas.getDimensions()[0]), mouseCoords[1] / (window.getWindowDimensions()[1] / lifeCanvas.getDimensions()[1]) };

	if (glfwGetMouseButton(window.getWindowInstance(), GLFW_MOUSE_BUTTON_LEFT)) {
		lifeCanvas.setCanvasPixel(static_cast<int>(scaledMouseCoords[0]), static_cast<int>(scaledMouseCoords[1]), std::array<unsigned char, 3> {255, 255, 255});
		window.updateWindow(lifeCanvas);
	}
	else if (glfwGetMouseButton(window.getWindowInstance(), GLFW_MOUSE_BUTTON_RIGHT)) {
		lifeCanvas.setCanvasPixel(static_cast<int>(scaledMouseCoords[0]), static_cast<int>(scaledMouseCoords[1]), std::array<unsigned char, 3> {0, 0, 0});
		window.updateWindow(lifeCanvas);
	}
	else if (glfwGetKey(window.getWindowInstance(), GLFW_KEY_C))
	{
		lifeCanvas.clearCanvas();
		window.updateWindow(lifeCanvas);
	}
}
