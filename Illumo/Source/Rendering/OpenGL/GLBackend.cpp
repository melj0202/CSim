#define STB_IMAGE_IMPLEMENTATION
#include "GLBackend.h"
#include "GLBuffer.h"
#include "GLDevice.h"
#include "GLShaderProgram.h"
#include "GLTexture.h"
#include "Logger.h"
#include <CommandQueue.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <IRenderWindow.h>
#include <IShaderProgram.h>
#include <RenderPass.h>
#include <cstdlib>
#include <cstring>
#include <string>

GLBackend::GLBackend(IRenderWindow* window)
{
  device = new GLDevice();
  commandQueue = new CommandQueue();
  this->window = window;
  Initialize();
}

GLBackend::~GLBackend() {}

void
GLBackend::Initialize()
{
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
}

void
GLBackend::BeginFrame()
{
}

void
GLBackend::EndFrame()
{
  window->swapBuffers();
  static long frameCount = 0;
  static double lastFpsTime = 0.0;
  frameCount++;
  double currentFpsTime = glfwGetTime();
  if (currentFpsTime - lastFpsTime >= 1.0) {
    this->fps =
      static_cast<int>(frameCount / (currentFpsTime - lastFpsTime) + 0.5);
    frameCount = 0;
    lastFpsTime = currentFpsTime;
  }
}

void
GLBackend::SubmitCommandQueue()
{
  GLResourceTables tables;
  tables.meshes = &_vaoRegistryLookup;
  tables.programs = &_programRegistryLookup;
  tables.textures = &_textureRegistryLookup;
  device->ExecuteCommandQueue(*commandQueue, tables);
}

void
GLBackend::PushToCommandQueue(RenderCommand command)
{
  commandQueue->Submit(command);
}

void
GLBackend::ClearCommandQueue()
{
  commandQueue->Reset();
}

void
GLBackend::Shutdown()
{
  for (std::unordered_map<unsigned long, std::unique_ptr<GLMesh>>::iterator it =
         _vaoRegistryLookup.begin();
       it != _vaoRegistryLookup.end();
       ++it) {
    if (it->second) {
      it->second->Destroy();
    }
  }
  _vaoRegistryLookup.clear();

  for (std::unordered_map<unsigned long,
                          std::unique_ptr<GLShaderProgram>>::iterator it =
         _programRegistryLookup.begin();
       it != _programRegistryLookup.end();
       ++it) {
    if (it->second) {
      it->second->Destroy();
    }
  }
  _programRegistryLookup.clear();

  for (std::unordered_map<unsigned long, std::unique_ptr<GLTexture>>::iterator
         it = _textureRegistryLookup.begin();
       it != _textureRegistryLookup.end();
       ++it) {
    if (it->second) {
      it->second->Destroy();
    }
  }
  _textureRegistryLookup.clear();

  delete device;
  device = nullptr;
  delete commandQueue;
  commandQueue = nullptr;
}

unsigned long
GLBackend::CreateMesh(const void* vertices,
                      size_t vertexSize,
                      const void* indices,
                      size_t indexSize,
                      unsigned long tableID)
{
  return CreateMesh(vertices,
                    vertexSize,
                    indices,
                    indexSize,
                    tableID,
                    MeshVertexLayout::Pos3Color3Uv2,
                    false);
}

unsigned long
GLBackend::CreateMesh(const void* vertices,
                      size_t vertexSize,
                      const void* indices,
                      size_t indexSize,
                      unsigned long tableID,
                      MeshVertexLayout layout,
                      bool dynamic)
{
  _vaoRegistryLookup[tableID] = std::make_unique<GLMesh>(
    vertices, vertexSize, indices, indexSize, layout, dynamic);
  return tableID;
}

unsigned long
GLBackend::CreateMesh(std::string filePath, unsigned long tableID)
{
  (void)filePath;
  Logger::LogWarning("CreateMesh(filePath) is not implemented");
  return tableID;
}

unsigned long
GLBackend::CreateShaderProgram(const ShaderPaths& paths, unsigned long tableID)
{
  _programRegistryLookup[tableID] = std::make_unique<GLShaderProgram>(paths);
  return tableID;
}

unsigned long
GLBackend::CreateShaderProgram(const ShaderSources& sources,
                               unsigned long tableID)
{
  _programRegistryLookup[tableID] = std::make_unique<GLShaderProgram>(sources);
  return tableID;
}

unsigned long
GLBackend::CreateTexture(const unsigned char* data,
                         const int width,
                         const int height,
                         unsigned long tableID)
{
  return CreateTexture(data, width, height, 4, tableID);
}

unsigned long
GLBackend::CreateTexture(const unsigned char* data,
                         const int width,
                         const int height,
                         int channels,
                         unsigned long tableID)
{
  _textureRegistryLookup[tableID] =
    std::make_unique<GLTexture>(data, width, height, channels);
  return tableID;
}

unsigned long
GLBackend::CreateTexture(const std::string& filePath, unsigned long tableID)
{
  _textureRegistryLookup[tableID] = std::make_unique<GLTexture>(filePath);
  return tableID;
}
