/*
    This class describes a RenderWindow that uses the Metal rendering API and Cocoa windowing api
     as Apple requires the its usage in the current versions of macOS since their support of 
     OpenGL is non existent.
*/
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <fstream>
#include <iostream>
#include <array>


class CocoaRenderWindow {
public:
    CocoaRenderWindow(const int width, const int height, const std::string &title);
    ~CocoaRenderWindow() { glfwTerminate();} //TODO: Add Metal cleanup code
    static void toggleFullscreen() { isFullscreen = !isFullscreen; };
    static GLFWwindow* getWindowInstance() { return window; };
    static void updateWindow();
    static std::array<double, 2> getMouseCoords();
    static std::array<int, 2> getWindowDimensions() { return std::array<int, 2> {windowWidth, windowHeight}; };
private:
    static std::array<double, 2> mouseCooods;
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