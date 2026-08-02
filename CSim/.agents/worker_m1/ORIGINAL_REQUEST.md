## 2026-06-12T14:34:37Z
Your task is to:
1. Fix all the C++ compilation and linker errors in the CSim project (R1, R2, R3).
2. Implement the scene frame-by-frame dispatch architecture (R2).
3. Fix the main loop exit in CellMain.cpp (R3).

Please refer to the following fix strategy derived from Explorer 1 and Explorer 3:
- Source/Util/Math.h: Add #pragma once, remove duplicate Matrix4 alias (line 13), define perspective inline wrapping glm::perspective.
- Source/System/ModuleObject.h: Make DirtyFlags an enum class, replace Vec3 with Vector3, add ObjectID id member to ModuleObject and update constructors to accept ObjectID id.
- Source/Core/Canvas.h: Inherit Canvas from Drawable<Canvas>, remove the incorrect : ModuleObject() call from the Canvas constructor initialization list.
- Source/Rendering/Scene.h: Include Rendering/Camera.h instead of Rendering/Camera2D.h. Declare missing member variables window (IRenderWindow*), nodeLookup (std::unordered_map<ObjectID, SceneObject*>), drawables (std::vector<DrawableBase*>). Declare and implement AddDrawable(DrawableBase*), ClearDrawables(), and Update() which should clear the OpenGL color and depth buffers (using glClearColor(0.1f, 0.1f, 0.1f, 1.0f) and glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)), and iterate and draw the active drawables. Note: Include <GL/glew.h> at the top of Scene.h to access glClear.
- Source/Rendering/SceneObject.h: Add Matrix4 transform = Matrix4(1.0f);. Add constructors SceneObject() : id(0) {} and SceneObject(ObjectID id) : id(id) {}.
- Source/Rendering/Renderer.h: Add missing semicolon after Scene* currentScene on line 18.
- Source/Rendering/Transform.h: Fix the syntax error by changing 'private' to 'private:' (or clear/delete the unused file if it conflicts or causes issues).
- Source/Rendering/Camera.h / Camera.cpp: Update constructor signature to match Illumo.cpp instantiation camera = std::make_unique<Camera>(glm::vec2(0.0f, 0.0f), 1.0f, envVars.get()); (i.e. constructor takes const glm::vec2&, float, IEnvVars*). Update constructor implementation to pass 0 to SceneObject constructor and set projectionType to ProjectonType::ORTOGRAPHIC by default.
- Source/Rendering/IBackend.h / GLBackend.h / GLBackend.cpp: Fix mismatches between IBackend interfaces and GLBackend implementation:
  - Add #include "IShaderProgram.h" to IBackend.h.
  - Update IBackend.h virtual methods to accept unsigned long tableID where needed, so it matches what Renderer.h calls:
    * virtual unsigned long CreateMesh(const void* vertices, size_t vertexSize, const void* indices, size_t indexSize, unsigned long tableID) = 0;
    * virtual unsigned long CreateMesh(std::string filePath, unsigned long tableID) = 0;
    * virtual unsigned long CreateShaderProgram(const ShaderPaths& paths, unsigned long tableID) = 0;
    * virtual unsigned long CreateShaderProgram(const ShaderSources& sources, unsigned long tableID) = 0;
    * virtual unsigned long CreateTexture(const unsigned char* data, const int width, const int height, unsigned long tableID) = 0;
    * virtual unsigned long CreateTexture(const std::string& filePath, unsigned long tableID) = 0;
  - Make sure GLBackend.h virtual methods override these with the exact same signatures.
  - In GLBackend.cpp:
    * Fix the double-defined GLBackend::CreateMesh. One should be CreateMesh with vertices/indices/tableID, and the other should be CreateMesh with std::string filePath and tableID (you can implement filePath version by just returning tableID or stubbing it if not used, but verify if it's called).
    * Fix GLBackend::BeginFrame() override to have no arguments in GLBackend.cpp (as declared in GLBackend.h).
    * Update GLMesh.h to add a constructor GLMesh(const void* vertices, size_t vertexSize, const void* indices, size_t indexSize) so that GLBackend.cpp can compile `std::make_unique<GLMesh>(vertices, vertexSize, indices, indexSize);`. Inside this constructor, convert vertices and indices raw pointers to vector data (assign to _vertexData and _indexData of the base class IMesh) and call glGenVertexArrays / glGenBuffers.
- Source/System/CellMain.cpp: Modify the infinite loop while (true) to check !illumo->ShouldClose().

Steps:
1. Run cmake -B build to configure the project.
2. Run cmake --build build to compile.
3. Identify any remaining errors and edit the files to fix them.
4. Run cmake --build build again until the build compiles and links cleanly.
5. Record the build command and results, and list the modified files.
