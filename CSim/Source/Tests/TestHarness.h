#pragma once
// Shared null window + canvas fixture for headless tests.

#include "Rendering/IRenderWindow.h"
#include "Rendering/Mock/MockBackend.h"
#include "Rendering/Renderer.h"
#include "Rendering/Camera.h"
#include "Game/Canvas.h"
#include "Services/EnvVars.h"
#include <array>
#include <string>

class NullRenderWindow : public IRenderWindow {
public:
	int width;
	int height;

	NullRenderWindow(int w, int h)
		: IRenderWindow(w, h, "test", nullptr)
		, width(w)
		, height(h)
	{
	}

	void updateWindow() override {}
	void toggleFullscreen() override {}
	void reinitializeWindow(const int, const int, const std::string&) override {}
	void reinitializeWindow() override {}
	void handleResize(int w, int h) override
	{
		width = w;
		height = h;
	}
	std::array<double, 2> getMouseCoords() override
	{
		return std::array<double, 2>{0.0, 0.0};
	}
	GLFWwindow* getWindowInstance() override { return nullptr; }
	std::array<int, 2> getWindowDimensions() override
	{
		return std::array<int, 2>{width, height};
	}
	bool shouldWindowClose() override { return false; }
	void swapBuffers() override {}
	void requestClose() override {}
};

// Owns mock renderer stack so Canvas/rules can run without GL.
struct HeadlessCanvasFixture {
	NullRenderWindow window;
	EnvVars env;
	Camera camera;
	MockBackend mock;
	Renderer renderer;
	Canvas* canvas;

	HeadlessCanvasFixture(int canvasW, int canvasH, int winW = 1280, int winH = 720)
		: window(winW, winH)
		, env()
		, camera(glm::vec2(0.0f, 0.0f), 1.0f, &env)
		, mock()
		, renderer(&window, &env, &camera, &mock, false)
		, canvas(nullptr)
	{
		env.setVar("WinX", winW);
		env.setVar("WinY", winH);
		env.setVar("CanvasX", canvasW);
		env.setVar("CanvasY", canvasH);
		mock.Initialize();
		canvas = new Canvas(canvasW, canvasH, &window, &camera, &renderer);
	}

	~HeadlessCanvasFixture()
	{
		delete canvas;
		canvas = nullptr;
	}

	// Game of Life convention in this codebase: 0 = alive, 1 = dead.
	static const unsigned char Alive = 0;
	static const unsigned char Dead = 1;

	void clearDead()
	{
		canvas->clearCanvas();
	}

	void setAlive(int x, int y)
	{
		canvas->setCanvasPixel(x, y, Alive);
	}

	void setDead(int x, int y)
	{
		canvas->setCanvasPixel(x, y, Dead);
	}

	unsigned char at(int x, int y)
	{
		return canvas->getCanvasPixel(x, y);
	}

	bool isAlive(int x, int y)
	{
		return at(x, y) == Alive;
	}
};
