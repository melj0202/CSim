# Assets

`Illumo/Assets/` contains first-party runtime files copied beside the
application. `Assets/RendererDemo/` contains the small Debug showcase atlas and
its managed contract-compatible sprite shader; fonts retain their existing
rendering path.

`Rendering/AssetManager.*` owns managed file textures and shaders:

- cache identity is canonical path plus texture loading options (or the shader
  path pair);
- acquisition returns a stable typed backend handle and increments an internal
  reference count on duplicates;
- a 2x2 checkerboard texture or fallback shader remains bound while the first
  request is pending or failed;
- one worker performs file reads and stb image decoding; `pump()` applies GPU
  create/replacement on the render thread before frame submission;
- Debug builds poll timestamps every 500 ms and coalesce requests; all builds
  support explicit reload;
- a failed reload preserves the last good GPU object and records the error;
- the final release destroys the resource, and shutdown cancels obsolete jobs
  and joins the worker before backend destruction.

This managed milestone covers textures and shaders. Dynamic quad meshes remain
renderer-owned; font atlases, model import, and general 3D mesh assets are not
part of it.
