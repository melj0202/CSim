#include "CreateOpenGLBackend.h"
#include "GLBackend.h"

std::unique_ptr<IBackend>
CreateOpenGLBackend(IRenderWindow* window)
{
  return std::make_unique<GLBackend>(window);
}
