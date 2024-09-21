#include "CellRuleSet.h"
#include "CellCanvas.h"

#define CELL_DEAD 0

int CellRuleSet::countNeighbors(const int r, const int c, const int w, const int h) const
{
	int i2 = 0;
	int j2 = 0;
	int count = 0;
	for (int i = r - 1; i <= r + 1; i++) {
		for (int j = c - 1; j <= c + 1; j++) {
			i2 = i;
			j2 = j;
			if (i == r && j == c) continue;
			if (i < 0) i2 = w - 1;
			if (j < 0) j2 = h - 1;
			if (i >= w) i2 = 0;
			if (j >= h) j2 = 0;
			if (!getCanvasPixel(i2, j2)) count++;
		}
	}
	return count;
}
