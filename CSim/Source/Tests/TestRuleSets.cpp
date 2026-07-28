// Headless cellular-automaton rules tests (no OpenGL window).

#include "Tests/TestHelpers.h"
#include "Tests/TestHarness.h"
#include "Rulesets/GameOfLifeRuleSet.h"
#include "Rulesets/SeedsRuleSet.h"
#include "Rulesets/BrainsBrainRuleSet.h"
#include "Rulesets/HighlifeRuleSet.h"
#include <cstdio>

static TestCounters g;

static void testGameOfLifeBlockStillLife()
{
	testSection("GoL: 2x2 block is still life");
	HeadlessCanvasFixture f(8, 8);
	f.clearDead();
	// Block
	f.setAlive(3, 3);
	f.setAlive(4, 3);
	f.setAlive(3, 4);
	f.setAlive(4, 4);

	GameOfLifeRuleSet rules(f.canvas);
	rules.calcGeneration(0, 0, 8, 8);

	testTrue(g, f.isAlive(3, 3) && f.isAlive(4, 3) && f.isAlive(3, 4) && f.isAlive(4, 4),
		"block cells remain alive");
	// Neighbors of block should stay dead
	testTrue(g, !f.isAlive(2, 3) && !f.isAlive(5, 3), "outside block still dead");
}

static void testGameOfLifeBlinker()
{
	testSection("GoL: blinker period-2 oscillator");
	HeadlessCanvasFixture f(8, 8);
	f.clearDead();
	// Horizontal blinker at row 4
	f.setAlive(3, 4);
	f.setAlive(4, 4);
	f.setAlive(5, 4);

	GameOfLifeRuleSet rules(f.canvas);
	rules.calcGeneration(0, 0, 8, 8);

	// Expect vertical
	testTrue(g, f.isAlive(4, 3) && f.isAlive(4, 4) && f.isAlive(4, 5),
		"blinker becomes vertical after 1 gen");
	testTrue(g, !f.isAlive(3, 4) && !f.isAlive(5, 4),
		"horizontal ends of blinker die");

	rules.calcGeneration(0, 0, 8, 8);
	testTrue(g, f.isAlive(3, 4) && f.isAlive(4, 4) && f.isAlive(5, 4),
		"blinker returns to horizontal after 2 gens");
	testTrue(g, !f.isAlive(4, 3) && !f.isAlive(4, 5),
		"vertical ends die on return");
}

static void testGameOfLifeEmptyStaysEmpty()
{
	testSection("GoL: empty grid stays empty");
	HeadlessCanvasFixture f(6, 6);
	f.clearDead();
	GameOfLifeRuleSet rules(f.canvas);
	rules.calcGeneration(0, 0, 6, 6);
	int alive = 0;
	for (int y = 0; y < 6; ++y)
	{
		for (int x = 0; x < 6; ++x)
		{
			if (f.isAlive(x, y))
			{
				++alive;
			}
		}
	}
	testEqInt(g, alive, 0, "no spontaneous births on empty grid");
}

static void testGameOfLifeEvalCellColors()
{
	testSection("GoL: evalCell colors");
	HeadlessCanvasFixture f(4, 4);
	GameOfLifeRuleSet rules(f.canvas);
	unsigned char rgb[3] = {1, 2, 3};
	rules.evalCell(HeadlessCanvasFixture::Dead, rgb);
	testTrue(g, rgb[0] == 255 && rgb[1] == 255 && rgb[2] == 255, "dead is white");
	rules.evalCell(HeadlessCanvasFixture::Alive, rgb);
	testTrue(g, rgb[0] == 0 && rgb[1] == 0 && rgb[2] == 0, "alive is black");
}

static void testSeedsBirthOnly()
{
	testSection("Seeds: birth on 2 neighbors, no survival");
	HeadlessCanvasFixture f(8, 8);
	f.clearDead();
	// Two adjacent alive cells create seeds pattern around them
	f.setAlive(3, 3);
	f.setAlive(4, 3);

	SeedsRuleSet rules(f.canvas);
	rules.calcGeneration(0, 0, 8, 8);

	// Original cells should die (Seeds has no survival)
	testTrue(g, !f.isAlive(3, 3) && !f.isAlive(4, 3), "seeds parents die");
	// Cells with exactly 2 live neighbors are born — e.g. (3,2) sees (3,3) and (4,3)
	testTrue(g, f.isAlive(3, 2) || f.isAlive(3, 4) || f.isAlive(4, 2) || f.isAlive(4, 4),
		"at least one birth from 2-neighbor rule");
}

static void testBriansBrainAliveBecomesDying()
{
	testSection("Brian's Brain: alive -> dying -> dead");
	HeadlessCanvasFixture f(6, 6);
	f.clearDead();
	f.setAlive(2, 2);

	BrainsBrainRuleSet rules(f.canvas);
	rules.calcGeneration(0, 0, 6, 6);
	// Isolated alive becomes dying (2)
	testEqUChar(g, f.at(2, 2), 2, "alive becomes dying");

	rules.calcGeneration(0, 0, 6, 6);
	testEqUChar(g, f.at(2, 2), HeadlessCanvasFixture::Dead, "dying becomes dead");
}

static void testHighlifeRuleTag()
{
	testSection("Highlife: rule tag");
	HeadlessCanvasFixture f(4, 4);
	HighlifeRuleSet rules(f.canvas);
	testTrue(g, rules.getRuleTag() == "HIGHLIFE", "Highlife rule tag");
}

int runRuleSetTests()
{
	g.failures = 0;
	std::printf("\n======== RuleSet tests ========\n");
	testGameOfLifeBlockStillLife();
	testGameOfLifeBlinker();
	testGameOfLifeEmptyStaysEmpty();
	testGameOfLifeEvalCellColors();
	testSeedsBirthOnly();
	testBriansBrainAliveBecomesDying();
	testHighlifeRuleTag();
	std::printf("======== RuleSet done (%d failure(s)) ========\n", g.failures);
	return g.failures;
}
