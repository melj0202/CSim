//
// Created by gravi on 10/6/2024.
//
#include "CellMain.h"
#include "Illumo.h"
#include "Game/CellGameModule.h"
#ifndef NDEBUG
#include "Engine/DebugModule.h"
#endif
#include "GLFW/glfw3.h"
#include "Logger.h"
#include <memory>

void CellMain(int argc, char** argv)
{
	Logger::initLogger();

	Illumo* illumo = new Illumo(argc, argv);

	// Services first (no Game includes inside Illumo).
	illumo->Init();

	// App owns composition: which modules ship in this product (D-E1).
	illumo->addModule(std::make_unique<CellGameModule>());
#ifndef NDEBUG
	// Debug console / FPS / quit keys — Debug builds only.
	illumo->addModule(std::make_unique<DebugModule>());
#endif
	illumo->StartModules();

	double lastTime = glfwGetTime();

	while (!illumo->ShouldClose())
	{
		double currentTime = glfwGetTime();
		double dt = currentTime - lastTime;
		lastTime = currentTime;

		illumo->Update(dt);
		illumo->Render();
	}

	illumo->Shutdown();

	delete illumo;

	Logger::LogTrace("Main loop finished");
	Logger::shutdownLogger();
}
