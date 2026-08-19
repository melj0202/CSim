# Illumo Rendering

The supported reusable path is:

```text
Drawable::AppendCommands -> Renderer -> CommandQueue -> IBackend
                                             |            |
                                             |            +-- private OpenGL
                                             +-- ordered tagged-union tokens
```

IllumoGame consumes public rendering contracts from `Illumo/Include/Illumo`.
OpenGL implementation headers remain under `Illumo/Source/Rendering/OpenGL` and
are private. `MockBackend` is exposed only by `Illumo::TestSupport`.

`Scene` is a non-owning ordered frame list, not a scene graph. Typed
slot+generation resource handles, the bounded command queue, managed
`AssetManager`, painter-correct `GameVisual`, transforms, sprites/animation,
text, and primitive-composed UI retain their existing behavior.

The Debug renderer demo proves assets, sprites, transforms, animation, and
reload through the same library path consumed by IllumoGame. D-E6 supersedes
the prior deferred-extraction rule: the public static-library boundary now
exists, but Illumo remains a focused library for a concrete simulator rather
than a speculative general-purpose engine.
