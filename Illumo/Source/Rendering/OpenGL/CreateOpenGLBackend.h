#pragma once

class IBackend;
class IRenderWindow;

// Production composition only: heap-allocate the OpenGL IBackend.
// Transfer ownership to Renderer with takeOwnership=true, or call Shutdown
// and delete when finished. Not for headless tests (use MockBackend inject).
IBackend*
CreateOpenGLBackend(IRenderWindow* window);
