// GameVisual primitive host: shapes + sprites via MockBackend (no OpenGL).

#include "Rendering/Camera.h"
#include "Rendering/Mock/MockBackend.h"
#include "Rendering/Primitives/GameVisual.h"
#include "Rendering/Renderer.h"
#include "Rendering/Scene.h"
#include "Services/EnvVars.h"
#include "Tests/TestHarness.h"
#include "Tests/TestHelpers.h"
#include "Tests/TestRegistry.h"

static TestCounters g;

static void
testGameVisualShapesEmitTokens()
{
  testSection("GameVisual: filled/outline/line shapes emit tokens");
  NullRenderWindow window(640, 480);
  EnvVars env;
  env.setVar("WinX", 640);
  env.setVar("WinY", 480);
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);

  GameVisual visual;
  visual.setWindow(&window);
  visual.setCamera(&camera);
  visual.setSpace(PrimitiveSpace::Pixels);
  visual.prepare(&renderer);

  ColorRgba red{ 255, 0, 0, 255 };
  ColorRgba green{ 0, 255, 0, 255 };
  ColorRgba blue{ 0, 0, 255, 255 };
  visual.addFilledRect(10.0f, 20.0f, 40.0f, 30.0f, red);
  visual.addOutlineRect(100.0f, 100.0f, 50.0f, 50.0f, green, 2.0f);
  visual.addLine(0.0f, 0.0f, 100.0f, 0.0f, blue, 2.0f);

  testEqSize(g, visual.shapeCount(), 3u, "three shapes stored");
  testTrue(
    g, renderer.getStyle(RenderStyleId::Shape) != nullptr, "Shape style");

  mock.resetCounters();
  Scene scene(&window, &camera);
  scene.AddDrawable(&visual, RenderLayerId::UI);

  renderer.BeginFrame();
  renderer.RenderScene(&scene, &camera);
  renderer.EndFrame();

  testTrue(
    g, mock.getLastNonEmptySubmittedCount() > 0, "shape frame non-empty");
  testEqSize(g,
             mock.countNonEmptyOfType(CommandType::UpdateBuffer),
             1u,
             "one shape buffer upload");
  testEqSize(g,
             mock.countNonEmptyOfType(CommandType::DrawIndexed),
             1u,
             "one shape draw batch");
  testTrue(g,
           mock.countNonEmptyOfType(CommandType::SetShader) >= 1u,
           "shape bindStyle sets shader");
  testTrue(g,
           mock.countNonEmptyOfType(CommandType::SetPipelineState) >= 1u,
           "pipeline state set");
}

static void
testGameVisualSpritesBatchByTexture()
{
  testSection("GameVisual: sprites batch by texture handle");
  NullRenderWindow window(800, 600);
  EnvVars env;
  env.setVar("WinX", 800);
  env.setVar("WinY", 600);
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);

  // Enroll two textures as content handles (owner side).
  unsigned long texA = renderer.allocateHandle();
  unsigned long texB = renderer.allocateHandle();
  unsigned char px[4] = { 255, 255, 255, 255 };
  renderer.enrollTexture(px, 1, 1, 4, texA);
  renderer.enrollTexture(px, 1, 1, 4, texB);

  GameVisual visual;
  visual.setWindow(&window);
  visual.prepare(&renderer);

  ColorRgba white{ 255, 255, 255, 255 };
  // Interleave textures so sort-by-handle is required for clean batches.
  visual.addSprite(texB, 0.0f, 0.0f, 16.0f, 16.0f, white);
  visual.addSprite(texA, 20.0f, 0.0f, 16.0f, 16.0f, white);
  visual.addSprite(texB, 40.0f, 0.0f, 16.0f, 16.0f, white);
  visual.addSprite(texA, 60.0f, 0.0f, 16.0f, 16.0f, white);

  testEqSize(g, visual.spriteCount(), 4u, "four sprites stored");
  testTrue(
    g, renderer.getStyle(RenderStyleId::Sprite) != nullptr, "Sprite style");

  mock.resetCounters();
  Scene scene(&window, &camera);
  scene.AddDrawable(&visual, RenderLayerId::World);

  renderer.BeginFrame();
  renderer.RenderScene(&scene, &camera);
  renderer.EndFrame();

  testEqSize(g,
             mock.countNonEmptyOfType(CommandType::UpdateBuffer),
             1u,
             "one sprite buffer upload");
  // Two texture groups → two SetTexture + two DrawIndexed (sorted A then B).
  testEqSize(g,
             mock.countNonEmptyOfType(CommandType::SetTexture),
             2u,
             "two texture binds after sort");
  testEqSize(g,
             mock.countNonEmptyOfType(CommandType::DrawIndexed),
             2u,
             "two sprite draw batches");
}

static void
testGameVisualCompositionHost()
{
  testSection("GameVisual: complex object composes host + primitives");
  // Stand-in for a game object that embeds GameVisual rather than owning
  // draw tokens itself.
  struct CrossMarker
  {
    GameVisual visual;
    void build(float cx, float cy, float arm)
    {
      visual.clearPrimitives();
      ColorRgba c{ 255, 200, 50, 255 };
      visual.addLine(cx - arm, cy, cx + arm, cy, c, 2.0f);
      visual.addLine(cx, cy - arm, cx, cy + arm, c, 2.0f);
      visual.addOutlineRect(
        cx - arm, cy - arm, arm * 2.0f, arm * 2.0f, c, 1.0f);
    }
  };

  NullRenderWindow window(320, 240);
  EnvVars env;
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);

  CrossMarker marker;
  marker.visual.setWindow(&window);
  marker.visual.prepare(&renderer);
  marker.build(160.0f, 120.0f, 20.0f);

  testEqSize(g, marker.visual.shapeCount(), 3u, "composed of three shapes");

  mock.resetCounters();
  testTrue(
    g, marker.visual.AppendCommands(&renderer), "composed visual emits tokens");
  // Direct AppendCommands leaves tokens in queue; EndFrame submits.
  renderer.EndFrame();
  testTrue(g,
           mock.countNonEmptyOfType(CommandType::DrawIndexed) >= 1u,
           "composed marker draws");
}

static void
testGameVisualTextPrimitive()
{
  testSection("GameVisual: text primitive emits shape batch");
  NullRenderWindow window(640, 480);
  EnvVars env;
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);

  GameVisual visual;
  visual.setWindow(&window);
  visual.prepare(&renderer);
  ColorRgba white{ 255, 255, 255, 255 };
  visual.addText("Hi", 10.0f, 20.0f, 12.0f, white);

  mock.resetCounters();
  testTrue(g, visual.AppendCommands(&renderer), "text AppendCommands");
  renderer.EndFrame();
  testTrue(g,
           mock.countNonEmptyOfType(CommandType::DrawIndexed) >= 1u,
           "text draws via shape mesh");
  testTrue(g,
           mock.countNonEmptyOfType(CommandType::UpdateBuffer) >= 1u,
           "text uploads shape buffer");
}

static void
testGameVisualEmptyAndInvisible()
{
  testSection("GameVisual: empty/invisible skip draws");
  NullRenderWindow window(320, 240);
  EnvVars env;
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);

  GameVisual visual;
  visual.prepare(&renderer);
  testTrue(g, visual.AppendCommands(&renderer), "empty succeeds");

  ColorRgba c{ 1, 2, 3, 255 };
  visual.addFilledRect(0, 0, 10, 10, c);
  visual.setVisible(false);
  mock.resetCounters();
  testTrue(g, visual.AppendCommands(&renderer), "invisible succeeds");
  renderer.EndFrame();
  testEqSize(g,
             mock.countNonEmptyOfType(CommandType::DrawIndexed),
             0u,
             "invisible emits no draw");
}

static int
runGameVisualCase(void (*testFunction)())
{
  g.failures = 0;
  testFunction();
  return g.failures;
}

void
registerGameVisualTests(IllumoTestRegistry& registry)
{
  registry.add("Illumo.GameVisual.Shapes", []() {
    return runGameVisualCase(testGameVisualShapesEmitTokens);
  });
  registry.add("Illumo.GameVisual.SpriteBatches", []() {
    return runGameVisualCase(testGameVisualSpritesBatchByTexture);
  });
  registry.add("Illumo.GameVisual.Composition", []() {
    return runGameVisualCase(testGameVisualCompositionHost);
  });
  registry.add("Illumo.GameVisual.Text",
               []() { return runGameVisualCase(testGameVisualTextPrimitive); });
  registry.add("Illumo.GameVisual.EmptyAndInvisible", []() {
    return runGameVisualCase(testGameVisualEmptyAndInvisible);
  });
}
