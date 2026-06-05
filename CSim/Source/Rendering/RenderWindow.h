#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <array>

#include "IRenderWIndow.h"
#include "System/IEnvVars.h"

//There will only ever be one instance of this class so it is ok to make it static

/*
	Im slowly starting to realize that maybe making it a singleton was a better idea...
	TODO: Make this class a singleton instead of a static class
*/




class RenderWindow : public IRenderWindow {
public:
	
	RenderWindow(const int width, const int height, const std::string &title, IEnvVars* envVars);
	~RenderWindow() { glfwTerminate(); };
	void reinitializeWindow(const int width, const int height, const std::string& title) override;
	void reinitializeWindow() override;
	void toggleFullscreen() override;
	GLFWwindow* getWindowInstance() override { return window; } ;
	//void bindKeyCallback(static std::function<void(GLFWwindow*, const int, int, const int, const int)> func);
	void updateWindow() override;
	std::array<double, 2> getMouseCoords() override;
	std::array<int, 2> getWindowDimensions() override { return std::array<int, 2> {windowWidth, windowHeight}; } ;
	void handleResize(int width, int height) override;
	bool shouldWindowClose() override;
	RenderQueue* getRenderQueue() override { return &renderQueue; } ;
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
	RenderQueue renderQueue;
};


