// Canvas domain / visual buffer tests (CPU-side; GPU is mock-enrolled only).

#include "Tests/TestHelpers.h"
#include "Tests/TestHarness.h"
#include "Rulesets/GameOfLifeRuleSet.h"
#include <cstdio>

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

static void testClearMarksUpload()
{
	testSection("Canvas: clear marks full R8 upload");
	HeadlessCanvasFixture f(4, 4);
	f.canvas->AppendCommands(&f.renderer);
	testTrue(g, !f.canvas->isTextureUploadPending(), "enroll upload drained");
	f.clearDead();
	testTrue(g, f.canvas->isTextureUploadPending(), "clear needs texture upload");
	const DirtyRect& ur = f.canvas->getUploadDirtyRegion();
	testTrue(g, ur.valid(), "clear upload rect valid");
	testEqInt(g, ur.width(), 4, "clear upload width");
	testEqInt(g, ur.height(), 4, "clear upload height");
}

static void testDefaultPalette()
{
	testSection("Canvas: default palette alive/dead colors");
	HeadlessCanvasFixture f(2, 2);
	const unsigned char* pal = f.canvas->getPaletteRgb();
	// state 0 = alive = black
	testEqUChar(g, pal[0], 0, "alive R");
	testEqUChar(g, pal[1], 0, "alive G");
	testEqUChar(g, pal[2], 0, "alive B");
	// state 1 = dead = white
	testEqUChar(g, pal[3], 255, "dead R");
	testEqUChar(g, pal[4], 255, "dead G");
	testEqUChar(g, pal[5], 255, "dead B");
}

static void testRebuildPaletteFromRules()
{
	testSection("Canvas: rebuildPalette from GameOfLife");
	HeadlessCanvasFixture f(4, 4);
	GameOfLifeRuleSet rules(f.canvas);
	f.canvas->rebuildPalette(&rules);
	testTrue(g, f.canvas->isPaletteUploadPending(), "palette upload pending after rebuild");
	const unsigned char* pal = f.canvas->getPaletteRgb();
	testEqUChar(g, pal[0], 0, "GoL alive black R");
	testEqUChar(g, pal[3], 255, "GoL dead white R");
	f.canvas->AppendCommands(&f.renderer);
	testTrue(g, !f.canvas->isPaletteUploadPending(), "palette upload cleared");
}

static void testEnrollCreatesGpuHandles()
{
	testSection("Canvas: init enrolls mesh/shader/cell+palette textures");
	HeadlessCanvasFixture f(4, 4);
	// mesh + shader + R8 cell + palette = at least 4 creates
	testTrue(g, f.mock.getCreateCount() >= 4u, "at least 4 create records");
}

static void testDirtyFlagsAndSparseUpload()
{
	testSection("Canvas: dirty flags + sparse R8 upload");
	HeadlessCanvasFixture f(4, 4);
	f.canvas->AppendCommands(&f.renderer);
	// Mirror CellGameModule::updateVisualTargets after the first frame.
	f.canvas->onTargetsRebuilt();
	testTrue(g, !f.canvas->isTextureUploadPending(), "upload clears pending");
	testTrue(g, !f.canvas->isCellsDirty(), "logical dirty cleared after rebuild");

	f.canvas->tickVisual(0.016f);
	testTrue(g, !f.canvas->isTextureUploadPending(), "tickVisual no-op does not dirty texture");

	f.setAlive(1, 1);
	testTrue(g, f.canvas->isCellsDirty(), "paint sets cellsDirty");
	testTrue(g, f.canvas->hasCellsDirtyRegion(), "paint sets sparse dirty region");
	testTrue(g, f.canvas->isTextureUploadPending(), "paint sets upload pending");
	const DirtyRect& r = f.canvas->getCellsDirtyRegion();
	testEqInt(g, r.minX, 1, "dirty minX");
	testEqInt(g, r.minY, 1, "dirty minY");
	testEqInt(g, r.maxX, 1, "dirty maxX");
	testEqInt(g, r.maxY, 1, "dirty maxY");
	const DirtyRect& ur = f.canvas->getUploadDirtyRegion();
	testEqInt(g, ur.minX, 1, "upload minX");
	testEqInt(g, ur.minY, 1, "upload minY");
}

int runCanvasDomainTests()
{
	g.failures = 0;
	std::printf("\n======== Canvas domain tests ========\n");
	testClearAndSetGet();
	testDimensions();
	testClearMarksUpload();
	testDefaultPalette();
	testRebuildPaletteFromRules();
	testEnrollCreatesGpuHandles();
	testDirtyFlagsAndSparseUpload();
	std::printf("======== Canvas domain done (%d failure(s)) ========\n", g.failures);
	return g.failures;
}
