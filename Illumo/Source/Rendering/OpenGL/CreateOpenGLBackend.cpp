#include "CreateOpenGLBackend.h"
#include "GLBackend.h"

IBackend*
CreateOpenGLBackend(IRenderWindow* window)
{
  return new GLBackend(window);
}
