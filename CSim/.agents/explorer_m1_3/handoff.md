# Handoff Report: CSim Milestone 1 Fix Strategy

This report details the findings and provides a precise fix strategy for resolving the compilation and linker errors in CSim for Milestone 1.

---

## 1. Observation

### Observation 1: `Source/Util/Math.h` Inclusion & Redefinition Issues
* **Missing Header Guard**: `Source/Util/Math.h` starts directly with `#include <glm/glm.hpp>` and has no `#pragma once` or `#ifndef` guards.
* **Duplicate Alias**:
  * Line 7: `using Matrix4 = glm::mat4;`
  * Line 13: `using Matrix4 = glm::mat4;`
* **Declared but Undefined function**:
  * Line 30: `Matrix4 perspective(float fov, float aspect, float near, float far);` (No definition in a corresponding `.cpp` file exists).

### Observation 2: Typo in `Source/System/ModuleObject.h`
* **Verbatim**:
  ```cpp
  15: struct Transform {
  16:     Vec3 position;
  17:     Vec3 scale;
  18:     Quaternion rotation;
  ```
  `Vec3` is used on lines 16 and 17. However, `Source/Util/Math.h` only defines `Vector3` (`using Vector3 = glm::vec3;`).

### Observation 3: Name Conflict in `Source/System/ModuleObject.h`
* **Verbatim**:
  ```cpp
  7: enum DirtyFlags : uint8_t {
  8:     None = 0,
  9:     Transform = 1 << 0,
  10:     Render = 1 << 1
  11: };
  ...
  15: struct Transform {
  ```
  Unscoped enum value `Transform` (line 9) conflicts with `struct Transform` (line 15) in the global namespace.

### Observation 4: `ObjectID id` in `ModuleObject` and `EntityTable`
* **Verbatim `ModuleObject.h`**:
  ```cpp
  28: struct ModuleObject {
  29: 
  30:     
  31:     unsigned long meshID;
  32:     unsigned long textureID;
  33:     Transform transform;    
  34:     
  35:     ModuleObject(unsigned long meshID, unsigned long textureID, Transform transform) : meshID(meshID), textureID(textureID), transform(transform) {}
  ...
  ```
  There is no `ObjectID id;` member variable in `ModuleObject`.
* **Verbatim `EntityTable.h`**:
  * Line 18: `objects.push_back(ModuleObject{ id });`
  * Line 33: `ObjectID lastID = objects[lastIndex].id;`
  * Line 44: `object.id = id;`
  These assume `ModuleObject` has a member `id` and constructors that initialize it.

### Observation 5: `Canvas` Inheritance and Constructor Calls
* **Verbatim `Canvas.h`**:
  * Line 19: `struct Canvas {` (Does not inherit from `Drawable<Canvas>`).
  * Line 23: `Canvas(int width, int height, IRenderWindow* window, Camera* camera) : ModuleObject() {` (Calls `ModuleObject()` but does not inherit from `ModuleObject`).
* **Verbatim `CellGameModule.cpp`**:
  * Line 332: `scene->AddDrawable(this->cellContext->getCellCanvas());` (Expects `Canvas` to be a subclass of `DrawableBase`/`Drawable<Canvas>`).

### Observation 6: `Scene.h` Mismatches
* **Verbatim `Scene.h`**:
  * Line 3: `#include "Rendering/Camera2D.h"` (No such file exists; it should be `Camera.h`).
  * Line 15: `Scene(IRenderWindow* window, Camera* camera) : window(window), activeCamera(camera) {` (Attempts to initialize non-existent member `window`).
  * Line 18: `nodeLookup[0] = root;` (Attempts to use non-existent member `nodeLookup`).
* **Verbatim `Illumo.cpp`**:
  * Line 129: `context.scene->ClearDrawables();`
  * Line 134: `scene->Update();`
  These call `ClearDrawables()` and `Update()` on `Scene`, which are not declared.
* **Verbatim `CellGameModule.cpp`**:
  * Line 332: `scene->AddDrawable(this->cellContext->getCellCanvas());` (Calls `AddDrawable` on `Scene`, which is not declared).

### Adjacent Observations: `SceneObject` and `Renderer` Errors
* **`SceneObject.h` Constructors & Members**:
  * `Camera::Camera(...)` in `Camera.cpp:6` calls `SceneObject(id)`.
  * `CommandLine::CommandLine(...)` in `CommandLine.cpp:18` implicitly calls `SceneObject()`.
  * `Scene` constructor in `Scene.h:16` calls `new SceneObject(0)`.
  * `Scene` constructor in `Scene.h:17` calls `root->transform = Matrix4();`.
  * However, `SceneObject.h` only declares `SceneObject(EntityTable* et)`. It lacks `SceneObject(ObjectID)`, `SceneObject()` constructors, and the `Matrix4 transform;` member variable.
* **`Renderer.h` Semicolon**:
  * Line 18: `Scene* currentScene` is missing a semicolon, which will fail compilation immediately.

---

## 2. Logic Chain

1. **Inclusion / Redefinition (`Math.h`)**:
   * Without `#pragma once`, including `Math.h` in multiple translation units (or multiple times in one) redefines type aliases and functions, causing redefinition errors.
   * Removing the duplicate alias `Matrix4` and adding `#pragma once` prevents redefinition.
   * Making `perspective` `inline` and implementing it using `glm::perspective` prevents linker errors.
2. **Vec3 Typo**:
   * `Math.h` aliases `glm::vec3` as `Vector3`. Replacing `Vec3` with `Vector3` fixes the compiler error in `ModuleObject.h`.
3. **DirtyFlags Conflict**:
   * Unscoped enum values reside in the enclosing scope. Declaring `enum class DirtyFlags : uint8_t` restricts the names to the `DirtyFlags` namespace, resolving the conflict between `DirtyFlags::Transform` and `struct Transform`.
4. **ObjectID/ModuleObject constructors**:
   * Adding `ObjectID id;` to `ModuleObject` and updating its constructors ensures that `EntityTable` can create `ModuleObject` instances with ids and access them.
5. **Canvas Inheritance & Constructor**:
   * `Canvas` must inherit from `Drawable<Canvas>` to allow implicit casting to `DrawableBase*` for `Scene::AddDrawable`.
   * Since `Canvas` does not inherit from `ModuleObject`, removing `: ModuleObject()` from its constructor initializer list resolves the compiler error.
6. **Scene & SceneObject Dependencies**:
   * Changing `#include "Rendering/Camera2D.h"` to `#include "Rendering/Camera.h"` fixes the missing header issue.
   * Declaring `window`, `nodeLookup`, `drawables`, `AddDrawable()`, `ClearDrawables()`, and `Update()` in `Scene.h` resolves missing member and method issues.
   * Adding default and ID-based constructors, and a `Matrix4 transform;` member to `SceneObject` allows subclasses (`Camera`, `CommandLine`) and `Scene` to construct and use `SceneObject` properly.
   * Adding a semicolon to `Scene* currentScene` in `Renderer.h` resolves a syntax error.

---

## 3. Caveats

* Investigations were performed using static read-only analysis without live compiler execution due to constraints.
* Assumptions were made that the codebase compiles using C++17 or higher (due to nested namespaces and features).
* It is assumed `Math::perspective` was intended to wrap `glm::perspective`.

---

## 4. Conclusion

Milestone 1 compilation and linker errors can be fully resolved by applying localized edits to:
1. `Source/Util/Math.h`
2. `Source/System/ModuleObject.h`
3. `Source/Core/Canvas.h`
4. `Source/Rendering/Scene.h`
5. `Source/Rendering/SceneObject.h`
6. `Source/Rendering/Renderer.h`

The exact edits required are detailed in the **Proposed Code Patches** section below.

---

## 5. Verification Method

To verify these fixes:
1. Apply the proposed patches to the source files.
2. Run the build system configuration (CMake) and build the project using MSBuild or `ninja` (e.g., `cmake -B build` followed by `cmake --build build`).
3. Verify that all translation units compile without errors and the executable links successfully.

---

## Proposed Code Patches (Fix Strategy)

### Patch 1: `Source/Util/Math.h`
* Add `#pragma once` to top.
* Remove duplicate `Matrix4` alias on line 13.
* Define `perspective` inline.

```diff
+ #pragma once
  #include <glm/glm.hpp>
  #include <glm/gtc/matrix_transform.hpp>
  #include <glm/gtc/quaternion.hpp>
  #include <algorithm>
  
  
  using Matrix4 = glm::mat4;
  using Vector2 = glm::vec2;
  using Vector3 = glm::vec3;
  using Vector4 = glm::vec4;
  using Matrix2 = glm::mat2;
  using Matrix3 = glm::mat3;
- using Matrix4 = glm::mat4;
  using Quaternion = glm::quat;
  ...
  namespace Math {
-     Matrix4 perspective(float fov, float aspect, float near, float far);
+     inline Matrix4 perspective(float fov, float aspect, float near, float far) {
+         return glm::perspective(fov, aspect, near, far);
+     }
```

### Patch 2: `Source/System/ModuleObject.h`
* Change `DirtyFlags` to `enum class`.
* Replace `Vec3` with `Vector3`.
* Add `ObjectID id;` and update constructors.

```diff
- enum DirtyFlags : uint8_t {
+ enum class DirtyFlags : uint8_t {
      None = 0,
      Transform = 1 << 0,
      Render = 1 << 1
  };
  
  using ObjectID = uint32_t;
  
  struct Transform {
-     Vec3 position;
-     Vec3 scale;
+     Vector3 position;
+     Vector3 scale;
      Quaternion rotation;
  
      Matrix4 toMatrix() {
          Matrix4 T = glm::translate(position);
          Matrix4 R = glm::mat4_cast(rotation);
          Matrix4 S = glm::scale(scale);
          return T * R * S;
      }
  };
  
  struct ModuleObject {
+     ObjectID id;
      unsigned long meshID;
      unsigned long textureID;
      Transform transform;    
      
-     ModuleObject(unsigned long meshID, unsigned long textureID, Transform transform) : meshID(meshID), textureID(textureID), transform(transform) {}
-     ModuleObject(unsigned long meshID, unsigned long textureID) : meshID(meshID), textureID(textureID), transform(Transform()) {}
-     ModuleObject(unsigned long meshID) : meshID(meshID), transform(Transform()) {}
-     ModuleObject() : meshID(0), textureID(0), transform(Transform()) {}
+     ModuleObject(ObjectID id, unsigned long meshID, unsigned long textureID, Transform transform) : id(id), meshID(meshID), textureID(textureID), transform(transform) {}
+     ModuleObject(ObjectID id, unsigned long meshID, unsigned long textureID) : id(id), meshID(meshID), textureID(textureID), transform(Transform()) {}
+     ModuleObject(ObjectID id, unsigned long meshID) : id(id), meshID(meshID), textureID(0), transform(Transform()) {}
+     ModuleObject(ObjectID id) : id(id), meshID(0), textureID(0), transform(Transform()) {}
+     ModuleObject() : id(0), meshID(0), textureID(0), transform(Transform()) {}
  };
```

### Patch 3: `Source/Core/Canvas.h`
* Inherit from `Drawable<Canvas>`.
* Remove `: ModuleObject()` constructor initializer.

```diff
- struct Canvas {
+ struct Canvas : public Drawable<Canvas> {
  	
  
  	public:
- 	Canvas(int width, int height, IRenderWindow* window, Camera* camera) : ModuleObject() {
+ 	Canvas(int width, int height, IRenderWindow* window, Camera* camera) {
  		this->window = window;
```

### Patch 4: `Source/Rendering/Scene.h`
* Fix include.
* Declare missing members `window`, `nodeLookup`, and `drawables`.
* Define `AddDrawable`, `ClearDrawables`, and `Update`.

```diff
- #include "Rendering/Camera2D.h"
+ #include "Rendering/Camera.h"
  #include "Rendering/Drawable.h"
  #include "Rendering/SceneObject.h"
  #include <unordered_map>
  #include "System/EntityTable.h"
  #include <tracy/Tracy.hpp>
  
  #include "RenderableObject.h"
  
  class Scene {
  
      public:
          Scene(IRenderWindow* window, Camera* camera) : window(window), activeCamera(camera) {
              root = new SceneObject(0);
              root->transform = Matrix4();
              nodeLookup[0] = root;
          };
          ~Scene() = default;
          
          EntityTable* entityTable;
+         IRenderWindow* window;
+         std::unordered_map<ObjectID, SceneObject*> nodeLookup;
+         std::vector<DrawableBase*> drawables;
+ 
+         void AddDrawable(DrawableBase* drawable) {
+             drawables.push_back(drawable);
+         }
+ 
+         void ClearDrawables() {
+             drawables.clear();
+         }
+ 
+         void Update() {
+             for (auto* drawable : drawables) {
+                 if (drawable) {
+                     drawable->Draw();
+                 }
+             }
+         }
  
          Camera* activeCamera;
          SceneObject* root;
          std::vector<RenderableObject> renderableObjects;
  
  };
```

### Patch 5: `Source/Rendering/SceneObject.h`
* Add `transform` member.
* Add constructors `SceneObject()` and `SceneObject(ObjectID)`.

```diff
  struct SceneObject {
      SceneObject* parent = nullptr;
      std::vector<SceneObject*> children;
      
      ObjectID id;
+     Matrix4 transform = Matrix4(1.0f);
+ 
+     SceneObject() : id(0) {}
+     SceneObject(ObjectID id) : id(id) {}
  
      SceneObject(EntityTable* et) {
          //Initalize parent to idenity matrix
          id = et->CreateEntity();
      }
```

### Patch 6: `Source/Rendering/Renderer.h`
* Add missing semicolon on line 18.

```diff
-     Scene* currentScene
+     Scene* currentScene;
```
