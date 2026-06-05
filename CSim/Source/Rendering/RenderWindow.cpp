#include "RenderWindow.h"
#include "thirdparty/stb/stb_easy_font.h"
#include "Logger.h"
#include "CommandLine.h"
#include <chrono>
#include "System/IEnvVars.h"
#include "System/InputManager.h"

void windowSizeCallback(GLFWwindow* window, int width, int height) noexcept
{

// 1. Grab our C++ instance pointer out of GLFW
	RenderWindow* myWindow = static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));

	// 2. Use that pointer instead of 'this'
	if (myWindow)
	{
		myWindow->handleResize(width, height);
	}
	Logger::LogTrace("Window resized to " + std::to_string(width) + "x" + std::to_string(height));
}

RenderWindow::RenderWindow(const int width, const int height, const std::string& title, IEnvVars* envVars) : IRenderWindow(width, height, title, envVars)
{

/*Init member variables*/

	this->mouseCoords = {0.0, 0.0};
	this->windowHeight = height;
	this->windowWidth = width;
	this->windowTitle = title;
	this->envVars = envVars;
	this->isFullScreen = envVars ? envVars->getVar("fullscreen").valueAsBool : false;
	this->window = nullptr;
	this->renderQueue = RenderQueue();

	/* Initialize the library */
	if (!glfwInit())
		std::exit(-1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	/* Create a windowed mode window and its OpenGL context */

	if (isFullScreen)
	{
		GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
		window = glfwCreateWindow(mode->width, mode->height, title.c_str(), primaryMonitor, nullptr);
	}
	else
	{
		window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	}
	Logger::LogTrace("Window created");
	if (!window)
	{
		if (!Logger::LogError("Failed to create window"))
		{
			std::cerr << "Failed to create window" << std::endl;
			std::cerr << "Exiting cleanly." << std::endl;
		}
		glfwTerminate();
		std::exit(-1);
	}
	glfwGetWindowSize(window, &windowWidth, &windowHeight);
	centerWindow();
	Logger::LogTrace("Window centered");

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glfwSetWindowUserPointer(window, this);
	glfwSetWindowSizeCallback(window, windowSizeCallback);
	glewExperimental = true;
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);

	// Set initial viewport size based on current framebuffer size
	int fbWidth = 0;
	int fbHeight = 0;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	glViewport(0, 0, fbWidth, fbHeight);
	Logger::LogTrace("Viewport set");

	/* Loop until the user closes the window */
	GLenum err = glewInit();
	Logger::LogTrace("Glew initialized");
	if (GLEW_OK != err)
	{
		Logger::LogError("Failed to initialize glew");
		std::exit(-1);
	}
	const GLubyte* versionGL = glGetString(GL_VERSION);
	std::string versionStr = versionGL ? reinterpret_cast<const char*>(versionGL) : "Unknown";
	std::string fullGLString = "OpenGL Context: " + versionStr;
	Logger::LogInfo(fullGLString.c_str());
	glfwSwapInterval(0);
	//END WINDOW CREATION
}

void RenderWindow::updateWindow()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderQueue.draw();
	glfwSwapBuffers(window);


}

std::array<double, 2> RenderWindow::getMouseCoords()
{
	glfwGetCursorPos(window, &mouseCoords[0], &mouseCoords[1]);
	return mouseCoords;
}

void RenderWindow::centerWindow()
{
//Get monitor dimensions
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	if (monitor)
	{
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);
		if (mode)
		{
			int monitorWidth = mode->width;
			int monitorHeight = mode->height;
			glfwSetWindowPos(window, (monitorWidth - windowWidth) / 2, (monitorHeight - windowHeight) / 2);
		}
	}
}

void RenderWindow::handleResize(int width, int height)
{
	windowWidth = width;
	windowHeight = height;

	int fbWidth = 0;
	int fbHeight = 0;
	glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
	glViewport(0, 0, fbWidth, fbHeight);
	if (envVars)
	{
		envVars->setVar("WinX", std::to_string(width));
		envVars->setVar("WinY", std::to_string(height));
	}
}

void RenderWindow::reinitializeWindow(const int width, const int height, const std::string& title)
{
//ServiceLocator::provide<IRenderWindow, RenderWindow>(width, height, title);
}

void RenderWindow::reinitializeWindow()
{
//glfwDestroyWindow(window);
//ServiceLocator::provide<IRenderWindow, RenderWindow>(windowWidth, windowHeight, windowTitle);
}

void RenderWindow::toggleFullscreen()
{
	if (isFullScreen)
	{
		GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
		glfwSetWindowMonitor(window, primaryMonitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		isFullScreen = false;
	}
	else
	{
		glfwSetWindowMonitor(window, NULL, 0, 0, windowWidth, windowHeight, 0);
		isFullScreen = true;
	}

}

bool RenderWindow::shouldWindowClose()
{
	return glfwWindowShouldClose(window);
}

void RenderWindow::requestClose()
{
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}