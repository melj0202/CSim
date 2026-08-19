#pragma once

#include <memory>

class IBackend;
class IRenderWindow;

// Constructs the production backend. Illumo owns its one Initialize call so
// production and injected factories follow the same fallible contract.
std::unique_ptr<IBackend>
CreateOpenGLBackend(IRenderWindow* window);
