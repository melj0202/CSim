#include "CellNormalState.h"

void CellNormalState::iterate(CellCanvas & lifeCanvas, CellRuleSet &ruleSet, RenderWindow &window)
{
	ruleSet.calcGeneration(0, 0, lifeCanvas.getDimensions()[0], lifeCanvas.getDimensions()[1], lifeCanvas);
	window.updateWindow(lifeCanvas);
}
