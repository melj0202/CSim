#include "CellEditState.h"
#include "CellCanvas.h"
#include "RenderWindow.h"
void CellEditState::iterate(CellRuleSet& ruleSet)
{
	std::array<double, 2> mouseCoords = RenderWindow::getMouseCoords();

	std::array<double, 2> scaledMouseCoords = { mouseCoords[0] / (RenderWindow::getWindowDimensions()[0] / getDimensions()[0]), mouseCoords[1] / (RenderWindow::getWindowDimensions()[1] / getDimensions()[1]) };
	if (glfwGetMouseButton(RenderWindow::getWindowInstance(), GLFW_MOUSE_BUTTON_RIGHT)) 
		setCanvasPixel(static_cast<int>(scaledMouseCoords[0]), static_cast<int>(scaledMouseCoords[1]), 1);
	else if (glfwGetMouseButton(RenderWindow::getWindowInstance(), GLFW_MOUSE_BUTTON_LEFT))
		setCanvasPixel(static_cast<int>(scaledMouseCoords[0]), static_cast<int>(scaledMouseCoords[1]), 0);
	else if (glfwGetKey(RenderWindow::getWindowInstance(), GLFW_KEY_C))
		clearCanvas();

	for (int i = 0; i < CellCanvas::canvasWidth * CellCanvas::canvasHeight; i++) {
		ruleSet.evalCell(CellCanvas::lifeCanvas[i], &CellCanvas::texCanvasBuffer[i * 3]);
	}
	RenderWindow::updateWindow();
}
