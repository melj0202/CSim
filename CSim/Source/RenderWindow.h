#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <iostream>
#include <functional>
#include <array>


//There will only ever be one instance of this class so it is ok to make it static
class RenderWindow {
public:

	RenderWindow(const int width, const int height, const std::string &title);
	~RenderWindow() { glfwTerminate(); };
	static void toggleFullscreen() { isFullScreen = !isFullScreen; };
	static GLFWwindow* getWindowInstance() { return window; };
	//void bindKeyCallback(static std::function<void(GLFWwindow*, const int, int, const int, const int)> func);
	static void updateWindow();
	static std::array<double, 2> getMouseCoords();
	static std::array <int, 2> getWindowDimensions() { return std::array<int, 2> {windowWidth, windowHeight}; };
private:
	static std::array<double, 2> mouseCoords;
	static GLFWwindow* window;
	static int windowWidth;
	static int windowHeight;
	static std::string windowTitle;
	static bool isFullScreen;
	


	static bool getShaderCompileStatus(const int shaderProgram);


	/*
		These are triangles that cover the screen.

		A texture is mapped on top of these traingles.

		This is where the state of the cell colony will be rendered
	*/
	static const std::array<float, 32> vertices;
	static const std::array<unsigned char, 32> indices;

};


