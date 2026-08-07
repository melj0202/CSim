#include "RenderWindow.h"
#include "CommandLine.h"
#include "Logger.h"
#include "Services/IEnvVars.h"
#include "Services/InputManager.h"
#include "thirdparty/stb/stb_easy_font.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <iostream>

void
windowSizeCallback(GLFWwindow* window, int width, int height) noexcept
{

  // 1. Grab our C++ instance pointer out of GLFW
  RenderWindow* myWindow =
    static_cast<RenderWindow*>(glfwGetWindowUserPointer(window));

  // 2. Use that pointer instead of 'this'
  if (myWindow) {
    myWindow->handleResize(width, height);
  }
  Logger::LogTrace("Window resized to " + std::to_string(width) + "x" +
                   std::to_string(height));
}

RenderWindow::RenderWindow(const int width,
                           const int height,
                           const std::string& title,
                           IEnvVars* envVars)
  : IRenderWindow(width, height, title, envVars)
{

  /*Init member variables*/

  this->mouseCoords = { 0.0, 0.0 };
  this->windowHeight = height;
  this->windowWidth = width;
  this->windowedX = 0;
  this->windowedY = 0;
  this->windowedWidth = width;
  this->windowedHeight = height;
  this->windowTitle = title;
  this->envVars = envVars;
  this->isFullScreen =
    envVars ? envVars->getVar("fullscreen").valueAsBool : false;
  this->window = nullptr;

  /* Initialize the library */
  if (!glfwInit())
    std::exit(-1);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  /* Create a windowed mode window and its OpenGL context */

  if (isFullScreen) {
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);
    window = glfwCreateWindow(
      mode->width, mode->height, title.c_str(), primaryMonitor, nullptr);
  } else {
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
  }
  Logger::LogTrace("Window created");
  if (!window) {
    Logger::LogError("Failed to create window");
    std::cerr << "Failed to create window" << std::endl;
    std::cerr << "Exiting cleanly." << std::endl;
    glfwTerminate();
    std::exit(-1);
  }
  glfwGetWindowSize(window, &windowWidth, &windowHeight);
  if (envVars) {
    envVars->setVar("WinX", std::to_string(windowWidth));
    envVars->setVar("WinY", std::to_string(windowHeight));
  }
  if (!isFullScreen) {
    centerWindow();
    glfwGetWindowPos(window, &windowedX, &windowedY);
    glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
  }
  Logger::LogTrace("Window centered");

  /* Make the window's context current */
  glfwMakeContextCurrent(window);
  glfwSetWindowUserPointer(window, this);
  glfwSetWindowSizeCallback(window, windowSizeCallback);
  glewExperimental = true;
  // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);

  // Set initial viewport size based on current framebuffer size
  int fbWidth = 0;
  int fbHeight = 0;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
  glViewport(0, 0, fbWidth, fbHeight);
  Logger::LogTrace("Viewport set");

  /* Loop until the user closes the window */
  GLenum err = glewInit();
  Logger::LogTrace("Glew initialized");
  if (GLEW_OK != err) {
    Logger::LogError("Failed to initialize glew");
    std::exit(-1);
  }
  const GLubyte* versionGL = glGetString(GL_VERSION);
  std::string versionStr =
    versionGL ? reinterpret_cast<const char*>(versionGL) : "Unknown";
  std::string fullGLString = "OpenGL Context: " + versionStr;
  Logger::LogInfo(fullGLString.c_str());
  glfwSwapInterval(0);
  // END WINDOW CREATION
}

void
RenderWindow::updateWindow()
{
  // Frame clear/draw/swap is owned by Renderer + Illumo::Render.
  // Kept as a no-op hook for any legacy call sites.
}

std::array<double, 2>
RenderWindow::getMouseCoords()
{
  glfwGetCursorPos(window, &mouseCoords[0], &mouseCoords[1]);
  return mouseCoords;
}

void
RenderWindow::centerWindow()
{
  // Get monitor dimensions
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  if (monitor) {
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (mode) {
      int monitorWidth = mode->width;
      int monitorHeight = mode->height;
      glfwSetWindowPos(window,
                       (monitorWidth - windowWidth) / 2,
                       (monitorHeight - windowHeight) / 2);
    }
  }
}

void
RenderWindow::handleResize(int width, int height)
{
  windowWidth = width;
  windowHeight = height;

  int fbWidth = 0;
  int fbHeight = 0;
  glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
  glViewport(0, 0, fbWidth, fbHeight);
  if (envVars) {
    envVars->setVar("WinX", std::to_string(width));
    envVars->setVar("WinY", std::to_string(height));
  }
}

void
RenderWindow::reinitializeWindow(const int width,
                                 const int height,
                                 const std::string& title)
{
  // ServiceLocator::provide<IRenderWindow, RenderWindow>(width, height, title);
}

void
RenderWindow::reinitializeWindow()
{
  // glfwDestroyWindow(window);
  // ServiceLocator::provide<IRenderWindow, RenderWindow>(windowWidth,
  // windowHeight, windowTitle);
}

void
RenderWindow::toggleFullscreen()
{
  if (isFullScreen) {
    glfwSetWindowMonitor(
      window, nullptr, windowedX, windowedY, windowedWidth, windowedHeight, 0);
    isFullScreen = false;
  } else {
    glfwGetWindowPos(window, &windowedX, &windowedY);
    glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode =
      primaryMonitor ? glfwGetVideoMode(primaryMonitor) : nullptr;
    if (primaryMonitor == nullptr || mode == nullptr) {
      Logger::LogError(
        "Cannot enter fullscreen: primary monitor is unavailable");
      return;
    }
    glfwSetWindowMonitor(window,
                         primaryMonitor,
                         0,
                         0,
                         mode->width,
                         mode->height,
                         mode->refreshRate);
    isFullScreen = true;
  }
  if (envVars != nullptr) {
    envVars->setVar("fullscreen", isFullScreen);
  }
}

bool
RenderWindow::shouldWindowClose()
{
  return glfwWindowShouldClose(window);
}

void
RenderWindow::requestClose()
{
  glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void
RenderWindow::swapBuffers()
{
  if (window != nullptr) {
    glfwSwapBuffers(window);
  }
}

RenderWindow::~RenderWindow()
{
  if (window != nullptr) {
    glfwDestroyWindow(window);
    window = nullptr;
  }
  glfwTerminate();
}
