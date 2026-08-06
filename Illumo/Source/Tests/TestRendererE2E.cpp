// End-to-end drawable → Renderer → MockBackend token tests (no OpenGL).
// Linked into the current IllumoTests target with TestMockBackend.cpp.

#include "Game/Canvas.h"
#include "Rendering/Camera.h"
#include "Rendering/Drawable.h"
#include "Rendering/IRenderWindow.h"
#include "Rendering/Mock/MockBackend.h"
#include "Rendering/Renderer.h"
#include "Rendering/Scene.h"
#include "Services/EnvVars.h"
#include "Tests/TestRegistry.h"
#include <cstdio>
#include <cstring>
#include <vector>

// Test helpers shared with TestMockBackend.cpp (same TU linkage: each file is
// separate). Duplicate minimal assert helpers — keep this file self-contained.

static int g_e2e_failures = 0;

static void
e2eTrue(bool cond, const char* msg)
{
  if (!cond) {
    std::printf("FAIL: %s\n", msg);
    ++g_e2e_failures;
  } else {
    std::printf("PASS: %s\n", msg);
  }
}

static void
e2eEqSize(size_t a, size_t b, const char* msg)
{
  if (a != b) {
    std::printf("FAIL: %s (got %zu, expected %zu)\n", msg, a, b);
    ++g_e2e_failures;
  } else {
    std::printf("PASS: %s\n", msg);
  }
}

static void
e2eEqInt(int a, int b, const char* msg)
{
  if (a != b) {
    std::printf("FAIL: %s (got %d, expected %d)\n", msg, a, b);
    ++g_e2e_failures;
  } else {
    std::printf("PASS: %s\n", msg);
  }
}

// ---------------------------------------------------------------------------
// Null window: dimensions only; no GLFW/GL
// ---------------------------------------------------------------------------
class E2ENullRenderWindow : public IRenderWindow
{
public:
  int width;
  int height;

  E2ENullRenderWindow(int w, int h)
    : IRenderWindow(w, h, "test", nullptr)
    , width(w)
    , height(h)
  {
  }

  void updateWindow() override {}
  void toggleFullscreen() override {}
  void reinitializeWindow(const int, const int, const std::string&) override {}
  void reinitializeWindow() override {}
  void handleResize(int w, int h) override
  {
    width = w;
    height = h;
  }
  std::array<double, 2> getMouseCoords() override
  {
    return std::array<double, 2>{ 0.0, 0.0 };
  }
  GLFWwindow* getWindowInstance() override { return nullptr; }
  std::array<int, 2> getWindowDimensions() override
  {
    return std::array<int, 2>{ width, height };
  }
  bool shouldWindowClose() override { return false; }
  void swapBuffers() override {}
  void requestClose() override {}
};

// ---------------------------------------------------------------------------
// Minimal token drawable for pure Renderer::RenderScene path
// ---------------------------------------------------------------------------
class TokenQuadDrawable : public DrawableBase
{
public:
  unsigned long meshHandle = 0;
  unsigned long shaderHandle = 0;
  unsigned long textureHandle = 0;
  bool enrolled = false;
  int appendCallCount = 0;
  int drawCount = 0;

  void enroll(Renderer* renderer)
  {
    float verts[32] = {
      1,  1,  0, 1, 0, 0, 1, 1, 1,  -1, 0, 0, 1, 0, 1, 0,
      -1, -1, 0, 0, 0, 1, 0, 0, -1, 1,  0, 1, 1, 0, 0, 1,
    };
    unsigned int idx[6] = { 0, 1, 2, 0, 2, 3 };
    meshHandle = renderer->allocateHandle();
    renderer->enrollMesh(verts, sizeof(verts), idx, sizeof(idx), meshHandle);

    ShaderSources sources;
    sources.vertexSource = "void main(){}";
    sources.fragmentSource = "void main(){}";
    shaderHandle = renderer->allocateHandle();
    renderer->enrollShader(sources, shaderHandle);

    unsigned char px[4] = { 255, 0, 255, 255 };
    textureHandle = renderer->allocateHandle();
    renderer->enrollTexture(px, 1, 1, 4, textureHandle);
    enrolled = true;
  }

  void Draw() override
  {
    // Should not be called if AppendCommands returns true.
    ++drawCount;
  }

  bool AppendCommands(Renderer* renderer) override
  {
    ++appendCallCount;
    if (!enrolled || !renderer || !isVisible()) {
      return true;
    }
    renderer->pushSetShader(shaderHandle);
    renderer->pushSetMesh(meshHandle);
    renderer->pushSetTexture(textureHandle, 0);
    float identity[16] = {
      1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
    };
    renderer->pushUniformMat4("uMVP", identity);
    renderer->pushUniformInt("ourTexture", 0);
    renderer->pushDrawIndexed(6, 0);
    return true;
  }
};

// Immediate-only drawable (returns false from AppendCommands default)
class ImmediateStubDrawable : public DrawableBase
{
public:
  int drawCount = 0;
  void Draw() override { ++drawCount; }
};

// ---------------------------------------------------------------------------
// Tests
// ---------------------------------------------------------------------------
static void
testRendererInjectsMockBackend()
{
  std::printf("\n--- e2e: Renderer inject MockBackend ---\n");
  E2ENullRenderWindow window(1280, 720);
  EnvVars env;
  env.setVar("WinX", 1280);
  env.setVar("WinY", 720);
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();

  Renderer renderer(&window, &env, &camera, &mock, false);
  e2eTrue(renderer.getBackend() == &mock, "getBackend is injected mock");
  e2eTrue(!renderer.ownsBackend(), "does not own injected backend");

  unsigned long h = renderer.allocateHandle();
  renderer.enrollMesh(nullptr, 64, nullptr, 0, h);
  e2eEqSize(mock.getCreateCount(), 1u, "enrollMesh hits mock CreateMesh");
  e2eEqSize(static_cast<size_t>(mock.getCreate(0).tableID),
            static_cast<size_t>(h),
            "create tableID matches handle");
}

static void
testRenderSceneTokenDrawable()
{
  std::printf("\n--- e2e: RenderScene + TokenQuadDrawable ---\n");
  E2ENullRenderWindow window(800, 600);
  EnvVars env;
  env.setVar("WinX", 800);
  env.setVar("WinY", 600);
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();

  Renderer renderer(&window, &env, &camera, &mock, false);
  Scene scene(&window, &camera);

  TokenQuadDrawable quad;
  quad.enroll(&renderer);
  scene.AddDrawable(&quad);

  ImmediateStubDrawable stub;
  scene.AddDrawable(&stub);

  renderer.BeginFrame();
  renderer.RenderScene(&scene, &camera);
  renderer.EndFrame();

  e2eEqInt(mock.getBeginFrameCount(), 1, "BeginFrame once");
  e2eEqInt(mock.getEndFrameCount(), 1, "EndFrame once");
  // RenderScene submits once (non-empty); EndFrame may submit empty.
  e2eTrue(mock.getSubmitCount() >= 1, "at least one submit");
  e2eTrue(mock.getLastNonEmptySubmittedCount() > 0,
          "non-empty token frame recorded");

  // Frame setup prefix
  const CommandType prefix[] = {
    CommandType::SetViewport,
    CommandType::SetPipelineState,
    CommandType::ClearScreen,
  };
  e2eTrue(mock.nonEmptyStartsWith(prefix, 3), "viewport/pipeline/clear prefix");

  e2eEqSize(mock.countNonEmptyOfType(CommandType::DrawIndexed),
            1u,
            "one DrawIndexed from token drawable");
  e2eEqSize(
    mock.countNonEmptyOfType(CommandType::SetShader), 1u, "one SetShader");
  e2eEqSize(mock.countNonEmptyOfType(CommandType::SetMesh), 1u, "one SetMesh");
  e2eEqSize(
    mock.countNonEmptyOfType(CommandType::SetTexture), 1u, "one SetTexture");

  // Viewport matches null window
  e2eEqInt(
    mock.getLastNonEmptySubmitted(0).viewport.width, 800, "viewport width 800");
  e2eEqInt(mock.getLastNonEmptySubmitted(0).viewport.height,
           600,
           "viewport height 600");

  // Hybrid: immediate stub still Draw()'d after token submit
  e2eEqInt(stub.drawCount, 1, "immediate stub Draw called once");
  e2eEqInt(quad.appendCallCount, 1, "token drawable AppendCommands once");
  e2eEqInt(quad.drawCount, 0, "token drawable Draw not called");

  // Enroll creates: mesh + shader + texture
  e2eTrue(mock.getCreateCount() >= 3u, "at least 3 create records from enroll");
}

static void
testRenderSceneCanvasTokens()
{
  std::printf("\n--- e2e: RenderScene + Canvas ---\n");
  E2ENullRenderWindow window(1280, 720);
  EnvVars env;
  env.setVar("WinX", 1280);
  env.setVar("WinY", 720);
  env.setVar("CanvasX", 16);
  env.setVar("CanvasY", 12);
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();

  Renderer renderer(&window, &env, &camera, &mock, false);
  Scene scene(&window, &camera);

  // Small canvas so enroll is cheap
  Canvas canvas(16, 12, &window, &camera, &renderer);
  scene.AddDrawable(&canvas);

  // Drain initial RGB display enroll upload.
  renderer.BeginFrame();
  renderer.RenderScene(&scene, &camera);
  renderer.EndFrame();
  e2eTrue(mock.countNonEmptyOfType(CommandType::UpdateTexture) >= 1u,
          "initial enroll uploads");

  // Paint → targets → snap → sparse RGB dirty-rect upload.
  canvas.setCanvasPixel(1, 1, 0);
  canvas.rebuildTargetsFromLife();
  canvas.setFadeSpeed(0.0f);
  canvas.setTargetColor(1 * canvas.canvasWidth + 1, 0, 0, 0);
  canvas.snapVisualToTargets();

  renderer.BeginFrame();
  renderer.RenderScene(&scene, &camera);
  renderer.EndFrame();

  e2eTrue(mock.getLastNonEmptySubmittedCount() > 0, "canvas frame non-empty");
  e2eEqSize(mock.countNonEmptyOfType(CommandType::UpdateTexture),
            1u,
            "Canvas UpdateTexture once when dirty");
  e2eEqSize(mock.countNonEmptyOfType(CommandType::DrawIndexed),
            1u,
            "Canvas DrawIndexed once");
  e2eEqSize(mock.countNonEmptyOfType(CommandType::SetUniformMat4),
            1u,
            "Canvas MVP uniform once");
  e2eEqSize(
    mock.countNonEmptyOfType(CommandType::ClearScreen), 1u, "frame clear once");

  // Canvas enroll: mesh + shader + RGB display
  e2eTrue(mock.getCreateCount() >= 3u, "Canvas enroll create records");

  bool foundRgbUpdate = false;
  for (size_t i = 0; i < mock.getLastNonEmptySubmittedCount(); ++i) {
    const RenderCommand& cmd = mock.getLastNonEmptySubmitted(i);
    if (cmd.commandType == CommandType::UpdateTexture) {
      e2eEqInt(cmd.updateTexture.x, 1, "UpdateTexture dirty x");
      e2eEqInt(cmd.updateTexture.y, 1, "UpdateTexture dirty y");
      e2eEqInt(cmd.updateTexture.width, 1, "UpdateTexture dirty width");
      e2eEqInt(cmd.updateTexture.height, 1, "UpdateTexture dirty height");
      e2eEqInt(cmd.updateTexture.channels, 3, "UpdateTexture RGB channels");
      e2eEqInt(cmd.updateTexture.srcRowStride,
               16,
               "UpdateTexture src row stride = canvas width");
      foundRgbUpdate = true;
    }
  }
  e2eTrue(foundRgbUpdate, "found UpdateTexture command");
}

static void
testRenderProofQuadOnMock()
{
  std::printf("\n--- e2e: RenderProofQuad via MockBackend ---\n");
  E2ENullRenderWindow window(640, 480);
  EnvVars env;
  Camera camera(glm::vec2(0.0f, 0.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);

  renderer.BeginFrame();
  renderer.RenderProofQuad();
  renderer.EndFrame();

  e2eTrue(mock.getLastNonEmptySubmittedCount() >= 8u,
          "proof quad emits several tokens");
  e2eEqSize(mock.countNonEmptyOfType(CommandType::DrawIndexed),
            1u,
            "proof DrawIndexed");
  e2eEqSize(mock.countNonEmptyOfType(CommandType::ClearScreen),
            1u,
            "proof ClearScreen");
  e2eTrue(mock.getCreateCount() >= 3u, "proof enrolls mesh/shader/texture");
}

static int
runRendererE2ECase(void (*testFunction)())
{
  g_e2e_failures = 0;
  testFunction();
  return g_e2e_failures;
}

void
registerRendererE2ETests(IllumoTestRegistry& registry)
{
  registry.add("Illumo.Renderer.InjectsMockBackend", []() {
    return runRendererE2ECase(testRendererInjectsMockBackend);
  });
  registry.add("Illumo.Renderer.SceneTokenDrawable", []() {
    return runRendererE2ECase(testRenderSceneTokenDrawable);
  });
  registry.add("Illumo.Renderer.CanvasTokens", []() {
    return runRendererE2ECase(testRenderSceneCanvasTokens);
  });
  registry.add("Illumo.Renderer.ProofQuad",
               []() { return runRendererE2ECase(testRenderProofQuadOnMock); });
}
