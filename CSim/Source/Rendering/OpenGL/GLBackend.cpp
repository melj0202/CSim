#define STB_IMAGE_IMPLEMENTATION
#include "GLBackend.h"
#include "GLBuffer.h"
#include "Logger.h"
#include <cstdlib>
#include <string>
#include <CommandQueue.h>
#include <IRenderWindow.h>
#include <IShaderProgram.h>
#include "GLDevice.h"
#include "GLShaderProgram.h"
#include "GLTexture.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <RenderPass.h>

GLBackend::GLBackend(IRenderWindow* window)
{
	device = new GLDevice();
	commandQueue = new CommandQueue();
	this->window = window;
	Initialize();
}
GLBackend::~GLBackend()
{
	
}

void GLBackend::Initialize()
{
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
}
void GLBackend::BeginFrame(RenderPass renderPass)
{
	
}

void GLBackend::EndFrame()
{
	window->swapBuffers();
	static long frameCount = 0;
	static double lastFpsTime = 0.0;
	frameCount++;
	double currentFpsTime = glfwGetTime();
	if (currentFpsTime - lastFpsTime >= 1.0)
	{
		double fps = frameCount / (currentFpsTime - lastFpsTime);

		frameCount = 0;
		lastFpsTime = currentFpsTime;
	}
}


void GLBackend::Shutdown()
{
	delete device;
	delete commandQueue;
}

unsigned long GLBackend::CreateMesh(const void* vertices, size_t vertexSize, const void* indices, size_t indexSize, unsigned long tableID)
{
	_vaoRegistryLookup[tableID] = std::make_unique<GLMesh>(vertices, vertexSize, indices, indexSize);
	return tableID;
}

unsigned long GLBackend::CreateMesh(const void* vertices, size_t vertexSize, const void* indices, size_t indexSize, unsigned long tableID)
{
	_vaoRegistryLookup[tableID] = std::make_unique<GLMesh>(vertices, vertexSize, indices, indexSize);
	return tableID;
}

unsigned long GLBackend::CreateShaderProgram(const ShaderPaths& paths, unsigned long tableID)
{
	_programRegistryLookup[tableID] = std::make_unique<GLShaderProgram>(paths);
	return tableID;
}

unsigned long GLBackend::CreateShaderProgram(const ShaderSources& sources, unsigned long tableID)
{
	_programRegistryLookup[tableID] = std::make_unique<GLShaderProgram>(sources);
	return tableID;
}


unsigned long GLBackend::CreateTexture(const unsigned char* data, const int width, const int height, unsigned long tableID)
{
	_textureRegistryLookup[tableID] = std::make_unique<GLTexture>(data, width, height);
	return tableID;
}

unsigned long GLBackend::CreateTexture(const std::string& filePath, unsigned long tableID)
{
	_textureRegistryLookup[tableID] = std::make_unique<GLTexture>(filePath);
	return tableID;
}