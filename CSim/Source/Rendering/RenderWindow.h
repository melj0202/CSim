#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <array>

#include "IRenderWindow.h"
#include "Services/IEnvVars.h"

// GLFW + GLEW window bootstrap. Allowed GL here is limited to context init
// (viewport on resize, version query). Frame draw goes through Renderer.
class RenderWindow : public IRenderWindow {
public:
	RenderWindow(const int width, const int height, const std::string& title, IEnvVars* envVars);
	~RenderWindow() { glfwTerminate(); }
	void reinitializeWindow(const int width, const int height, const std::string& title) override;
	void reinitializeWindow() override;
	void toggleFullscreen() override;
	GLFWwindow* getWindowInstance() override { return window; }
	void updateWindow() override;
	std::array<double, 2> getMouseCoords() override;
	std::array<int, 2> getWindowDimensions() override
	{
		return std::array<int, 2>{windowWidth, windowHeight};
	}
	void handleResize(int width, int height) override;
	bool shouldWindowClose() override;
	void swapBuffers() override { glfwSwapBuffers(window); }
	void requestClose() override;

private:
	std::array<double, 2> mouseCoords;
	GLFWwindow* window;
	IEnvVars* envVars;
	int windowWidth;
	int windowHeight;
	std::string windowTitle;
	bool isFullScreen;
	void centerWindow();
	bool getShaderCompileStatus(const unsigned int shaderProgram);
};
