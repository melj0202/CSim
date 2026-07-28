// Canvas domain / visual buffer tests (CPU-side; GPU is mock-enrolled only).

#include "Tests/TestHelpers.h"
#include "Tests/TestHarness.h"
#include <cstdio>
#include <cmath>

static TestCounters g;

static void testClearAndSetGet()
{
	testSection("Canvas: clear + set/get pixel");
	HeadlessCanvasFixture f(8, 6);
	f.clearDead();
	testEqUChar(g, f.at(0, 0), HeadlessCanvasFixture::Dead, "cleared cell is dead");
	f.setAlive(2, 3);
	testEqUChar(g, f.at(2, 3), HeadlessCanvasFixture::Alive, "set alive");
	testEqUChar(g, f.at(1, 3), HeadlessCanvasFixture::Dead, "neighbor unchanged");
	f.setDead(2, 3);
	testEqUChar(g, f.at(2, 3), HeadlessCanvasFixture::Dead, "set dead");
}

static void testDimensions()
{
	testSection("Canvas: dimensions");
	HeadlessCanvasFixture f(16, 10);
	std::array<int, 2> d = f.canvas->getDimensions();
	testEqInt(g, d[0], 16, "width");
	testEqInt(g, d[1], 10, "height");
}

static void testClearSnapsVisualWhite()
{
	testSection("Canvas: clear snaps visual buffers to white");
	HeadlessCanvasFixture f(4, 4);
	// Dirty a target color then clear
	f.canvas->setTargetColor(0, 0, 0, 0);
	f.canvas->clearCanvas();
	const int n = 4 * 4 * 3;
	bool allWhite = true;
	for (int i = 0; i < n; ++i)
	{
		if (f.canvas->texCanvasBuffer[i] != 255)
		{
			allWhite = false;
			break;
		}
	}
	testTrue(g, allWhite, "texCanvasBuffer all 255 after clear");
}

static void testSnapVisualToTargets()
{
	testSection("Canvas: snapVisualToTargets");
	HeadlessCanvasFixture f(2, 2);
	f.canvas->setTargetColor(0, 10, 20, 30);
	f.canvas->setTargetColor(1, 255, 128, 0);
	f.canvas->snapVisualToTargets();
	testEqUChar(g, f.canvas->texCanvasBuffer[0], 10, "cell0 R");
	testEqUChar(g, f.canvas->texCanvasBuffer[1], 20, "cell0 G");
	testEqUChar(g, f.canvas->texCanvasBuffer[2], 30, "cell0 B");
	testEqUChar(g, f.canvas->texCanvasBuffer[3], 255, "cell1 R");
	testEqUChar(g, f.canvas->texCanvasBuffer[4], 128, "cell1 G");
	testEqUChar(g, f.canvas->texCanvasBuffer[5], 0, "cell1 B");
}

static void testTickVisualMovesTowardTarget()
{
	testSection("Canvas: tickVisual lerps toward target");
	HeadlessCanvasFixture f(2, 2);
	// After clear, display is white (1.0). Target black for cell 0.
	f.canvas->setTargetColor(0, 0, 0, 0);
	f.canvas->setFadeSpeed(8.0f);
	f.canvas->tickVisual(0.5f);
	// Should have moved toward black (tex < 255)
	testTrue(g, f.canvas->texCanvasBuffer[0] < 255, "R decreased toward black");
	testTrue(g, f.canvas->texCanvasBuffer[1] < 255, "G decreased toward black");
	// Instant fade
	f.canvas->setFadeSpeed(0.0f);
	f.canvas->setTargetColor(0, 0, 0, 0);
	f.canvas->tickVisual(0.01f);
	testEqUChar(g, f.canvas->texCanvasBuffer[0], 0, "fadeSpeed 0 snaps R");
}

static void testEnrollCreatesGpuHandles()
{
	testSection("Canvas: init enrolls mesh/shader/texture on mock");
	HeadlessCanvasFixture f(4, 4);
	// Construction already enrolled
	testTrue(g, f.mock.getCreateCount() >= 3u, "at least 3 create records");
}

int runCanvasDomainTests()
{
	g.failures = 0;
	std::printf("\n======== Canvas domain tests ========\n");
	testClearAndSetGet();
	testDimensions();
	testClearSnapsVisualWhite();
	testSnapVisualToTargets();
	testTickVisualMovesTowardTarget();
	testEnrollCreatesGpuHandles();
	std::printf("======== Canvas domain done (%d failure(s)) ========\n", g.failures);
	return g.failures;
}
