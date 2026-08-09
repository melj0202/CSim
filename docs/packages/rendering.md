# Rendering

Illumo's reusable 2D front end keeps the existing boundary:

```text
Drawable::AppendCommands -> Renderer -> CommandQueue -> IBackend
                                             |            |
                                             |            +-- OpenGL or Mock
                                             +-- ordered tagged-union tokens
```

`Renderer.h` depends on `IBackend`, never concrete OpenGL types. `Scene` is a
non-owning ordered frame list with World, UI, and Debug layers; it is not a
scene graph or render graph.

## Resources and styles

`MeshHandle`, `ShaderHandle`, `TextureHandle`, and `RenderStyleHandle` are
non-convertible slot+generation values. Backends allocate them and validate
replace, destroy, validity, metadata, and submitted resource commands. Stale
operations warn and safely no-op.

Renderer owns a generational style registry. Canvas, shape, sprite, UI text,
and console styles are registered built-ins. A custom `GameVisual` 2D style may
consume `uMVP` (`mat4`), `uUsePixels` (`int`), and `u_resolution` (`vec2`);
sprite styles additionally consume sampler `uTexture` on texture unit zero.
Unused uniforms may be omitted by a custom shader.

The vector command queue reserves 2,048 entries, grows as needed, and rejects
only at a configurable default ceiling of 65,536. It exposes high-water,
per-frame rejection, and lifetime rejection counts.

## Painter-correct primitives

`GameVisual` builds one stable stream across shape, sprite, and text items,
ordered by integer `drawOrder` and insertion sequence. It batches only adjacent
compatible items; it never globally sorts transparent sprites by texture.

Shapes and sprites have local `Transform2D`; the host supplies one parent
transform. Position, scale, radians rotation, and normalized pivot are
supported without a hierarchy. The default `(0,0)` pivot preserves top-left
behavior. Sprites support normalized `TextureRegion`, grid-cell helpers,
horizontal/vertical flips, and centered convenience creation.

Dynamic shape/sprite buffers begin at 1,024 quads, double as needed, and stop at
a configurable 65,536 default ceiling. `SpriteAnimator` is passive: callers
advance Once, Loop, or PingPong clips outside submission, and rendering observes
only the current region.

## Primitive-composed UI

`UiTheme` supplies shared value-only colors and compact panel chrome. It does
not own widgets or layout. `CommandLine` composes its surface, inset history,
input row, selection, caret, scrollbar, and floating resize affordance directly
from `GameVisual` fills, outlines, lines, and text. `GLString` remains a cached
text drawable and can optionally add panel shadow, surface, border, and accent
primitives; the FPS badge and fading EDIT/NORMAL notice use that path.

All of these drawables retain their existing owners and Scene layers. The
console and each decorated label remain one adjacent shape/text batch, so the
redesign does not add a widget tree or per-element Scene traffic.

## Product boundary

The Debug-only `renderer_demo`, `assets`, and `asset_reload` commands prove the
foundation inside Illumo. The showcase acquires its atlas and custom sprite
shader through `AssetManager`, so successful texture reload and failed-shader
retention are observable in the product. Illumo remains a cellular-automata
application with a reusable 2D renderer, not a general engine. Extract a
standalone library only when a second real project demonstrates the required
public boundary.
