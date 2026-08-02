## 2026-06-12T14:31:01Z
<USER_REQUEST>
Analyze the CSim codebase and formulate a precise fix strategy for Milestone 1 (Resolve C++ Compilation and Linker Errors).
Your working directory is: c:\Users\gravi\Source\Projects\CSim - Copy\CSim\.agents\explorer_m1_1
Verify:
1. Duplicate template definitions in Source/Util/Math.h.
2. Vec3 typo in Source/System/ModuleObject.h.
3. DirtyFlags name conflicts with struct Transform in Source/System/ModuleObject.h.
4. ObjectID id member in ModuleObject, its constructors, and its usage in EntityTable.
5. Canvas inheritance from Drawable<Canvas> and the incorrect ModuleObject() call in the Canvas constructor.
6. Scene.h include changes (Camera2D.h -> Camera.h) and missing member variables window and nodeLookup.

Formulate a precise strategy and report back. Do NOT write or edit source code files.
</USER_REQUEST>
