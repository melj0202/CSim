#include "CellNormalState.h"
#include "CellCanvas.h"
#include "RenderWindow.h"

void CellNormalState::iterate(CellRuleSet &ruleSet)
{
	ruleSet.calcGeneration(0, 0, getDimensions()[0], getDimensions()[1]);
	for (int i = 0; i < CellCanvas::canvasWidth * CellCanvas::canvasHeight; i++) {
		ruleSet.evalCell(CellCanvas::lifeCanvas[i], &CellCanvas::texCanvasBuffer[i * 3]);
	}
	RenderWindow::updateWindow();
}
