//
// Created by gravi on 10/6/2024.
//
#include "CellMain.h"
#include "Illumo.h"
#include "GLFW/glfw3.h"
#include "Logger.h"
// Define global variables

void CellMain(int argc, char** argv)
{

	Logger::initLogger();

	Illumo* illumo = new Illumo(argc, argv);

	illumo->Init();
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
