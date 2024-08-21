#pragma once
#include <Windows.h>
#include <GL/glew.h>
#include <gl/GL.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <iostream>
#include "CellCanvas.h"
#include <functional>

class RenderWindow {
public:

	RenderWindow(const int width, const int height, const std::string &title, const CellCanvas &canvas) : windowWidth(width), windowHeight(height), windowTitle(title) {};
	void toggleFullscreen() { isFullScreen = !isFullScreen; };
private:

	enum class application_mode {
		NORMAL, EDIT
	};
	GLFWwindow* window;
	int windowWidth;
	int windowHeight;
	std::string windowTitle;
	bool isPaused = false;
	bool isFullScreen = false;
	bool start_sim = false;
	application_mode currentMode = application_mode::NORMAL;
	bool getShaderCompileStatus(const int shaderProgram);
	void bindKeyCallback(static std::function<void(GLFWwindow*, const int, int, const int, const int)> func);


	/*
		These are triangles that cover the screen.

		A texture is mapped on top of these traingles.

		This is where the state of the cell colony will be rendered
	*/
	const std::array<float, 32> vertices = {
		// positions          // colors           // texture coords
		 1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
		 1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
		-1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
	};
	const std::array<unsigned char, 32> indices = {
		0, 1, 3,
		1, 2, 3

	};

};