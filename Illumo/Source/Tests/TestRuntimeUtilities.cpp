#include "Engine/IllumoContext.h"
#include "Rendering/AssetManager.h"
#include "Rendering/BackendConfig.h"
#include "Rendering/PresentationTiming.h"
#include "Rendering/SplashText.h"
#include "Services/InputContext.h"
#include "Services/SysCmdLine.h"
#include "Tests/TestAccess.h"
#include "Tests/TestHarness.h"
#include "Tests/TestHelpers.h"
#include "Tests/TestRegistry.h"
#include <GLFW/glfw3.h>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static TestCounters g;

static void
testInputContextBindings()
{
  testSection("InputContext: action bindings");
  InputContext context;
  InputEvent event;
  event.keyCode = KeyCode::A;
  event.inputAction = InputAction::Press;
  context.bindAction("primary", event);
  testEqSize(g, context.getActions().size(), 1, "binding is stored");
  testTrue(g,
           context.getActionTag("primary").keyCode == KeyCode::A,
           "bound key is returned");

  event.keyCode = KeyCode::B;
  event.inputAction = InputAction::Hold;
  context.bindAction("primary", event);
  testEqSize(
    g, context.getActions().size(), 1, "rebinding replaces existing action");
  testTrue(g,
           context.getActionTag("primary").inputAction == InputAction::Hold,
           "rebound action is returned");

  bool threw = false;
  try {
    (void)context.getActionTag("missing");
  } catch (const std::out_of_range&) {
    threw = true;
  }
  testTrue(g, threw, "unknown action reports out_of_range");
}

static void
testInputManagerMappings()
{
  testSection("InputManager: key and action translation");
  InputManager input(nullptr);
  for (int value = static_cast<int>(KeyCode::Space);
       value <= static_cast<int>(KeyCode::MouseButton8);
       ++value) {
    const KeyCode key = static_cast<KeyCode>(value);
    const int glfwKey = InputManagerTestAccess::toGlfw(input, key);
    testTrue(g, glfwKey != -1, "known key maps to GLFW");
    testTrue(g,
             InputManagerTestAccess::fromGlfw(input, glfwKey) == key,
             "GLFW key maps back to engine key");
  }
  testEqInt(g,
            InputManagerTestAccess::toGlfw(input, KeyCode::None),
            -1,
            "None has no GLFW token");
  testTrue(g,
           InputManagerTestAccess::fromGlfw(input, -999) == KeyCode::None,
           "unknown GLFW token maps to None");
  testTrue(g,
           InputManagerTestAccess::actionFromGlfw(input, GLFW_PRESS) ==
             InputAction::Press,
           "press action maps");
  testTrue(g,
           InputManagerTestAccess::actionFromGlfw(input, GLFW_RELEASE) ==
             InputAction::Release,
           "release action maps");
  testTrue(g,
           InputManagerTestAccess::actionFromGlfw(input, GLFW_REPEAT) ==
             InputAction::Hold,
           "repeat action maps to hold");
  testTrue(g,
           InputManagerTestAccess::actionFromGlfw(input, -1) ==
             InputAction::Release,
           "unknown action safely maps to release");
}

static void
testInputManagerHeadlessLifecycle()
{
  testSection("InputManager: null-window lifecycle and callbacks");
  InputManager input(nullptr);
  testTrue(g,
           input.GetInputAction(KeyCode::None) == InputAction::None,
           "None action is inert");
  testTrue(g,
           input.GetInputAction(KeyCode::A) == InputAction::None,
           "null-window key action is inert");
  testTrue(
    g, !input.isKeyPressed(KeyCode::A), "null-window key press is false");
  testTrue(
    g, !input.isKeyReleased(KeyCode::A), "null-window key release is false");
  testTrue(g,
           !input.isMouseButtonPressed(KeyCode::MouseLeft),
           "null-window mouse press is false");
  testTrue(g,
           !input.isMouseButtonReleased(KeyCode::MouseLeft),
           "null-window mouse release is false");
  const std::array<double, 2> mouse = input.getMousePosition();
  testTrue(g,
           mouse[0] == 0.0 && mouse[1] == 0.0,
           "null-window mouse position is origin");

  InputManager::characterCallback(nullptr, static_cast<unsigned int>('x'));
  InputManager::normalKeyCallback(nullptr, GLFW_KEY_A, 0, GLFW_PRESS, 2);
  InputManager::scrollCallback(nullptr, 0.0, -3.5);
  testEqSize(
    g, input.getCharQueue().size(), 1, "character callback queues input");
  testEqSize(g, input.getKeyQueue().size(), 1, "key callback queues input");
  testTrue(g,
           input.getKeyQueue().front().key == KeyCode::A,
           "queued key is translated");
  testTrue(
    g, *input.getMouseScrollOffset() == -3.5, "scroll callback stores offset");
  input.clearCharQueue();
  input.clearKeyQueue();
  testEqSize(g, input.getCharQueue().size(), 0, "character queue clears");
  testEqSize(g, input.getKeyQueue().size(), 0, "key queue clears");

  input.update();
  testTrue(
    g, *input.getMouseScrollOffset() == 0.0, "update resets scroll offset");
  testTrue(g,
           input.GetInputAction(KeyCode::A) == InputAction::None,
           "headless update remains inert");
}

static void
testInputManagerContextsAndCapacity()
{
  testSection("InputManager: contexts, action lookup, and capacity");
  InputManager input(nullptr);
  InputContext first;
  InputEvent event;
  event.keyCode = KeyCode::E;
  event.inputAction = InputAction::Press;
  first.bindAction("toggle", event);
  const long firstId = input.registerInputContext(first);
  testEqInt(g, static_cast<int>(firstId), 0, "first context receives id zero");
  input.setActiveInputContext(firstId);
  testTrue(g,
           input.getActiveInputContext()->getActionTag("toggle").keyCode ==
             KeyCode::E,
           "active context is selected");
  InputManagerTestAccess::setAction(input, KeyCode::E, InputAction::Press);
  testTrue(
    g, input.isActionActive("toggle"), "bound action matches cached state");
  InputManagerTestAccess::setAction(input, KeyCode::E, InputAction::Hold);
  testTrue(
    g, !input.isActionActive("toggle"), "different cached state is inactive");

  for (int i = 1; i < NUM_INPUT_CONTEXTS; ++i) {
    testTrue(g,
             input.registerInputContext(InputContext()) >= 0,
             "context registers below capacity");
  }
  testEqInt(g,
            static_cast<int>(input.registerInputContext(InputContext())),
            -1,
            "context capacity is enforced");
}

static void
testBackendConfigTokens()
{
  testSection("BackendConfig: environment token conversion");
  EnvVars env;
  const BackendDef definitions[] = { BackendDef::OPENGL,
                                     BackendDef::OPENGL_ES,
                                     BackendDef::VULKAN,
                                     BackendDef::DIRECTX12,
                                     BackendDef::DIRECTX11 };
  for (BackendDef definition : definitions) {
    const std::string token = TokenToString(definition);
    env.setVar("GraphicsAPI", token);
    testTrue(g,
             StringToToken(&env) == definition,
             "graphics backend token round-trips");
  }
  env.setVar("GraphicsAPI", "UNKNOWN");
  testTrue(g,
           StringToToken(&env) == BackendDef::OPENGL,
           "unknown backend falls back to OpenGL");
  testTrue(g,
           TokenToString(static_cast<BackendDef>(999)) == "OPENGL",
           "unknown enum falls back to OpenGL");
}

static void
testPresentationTimingPolicy()
{
  testSection("PresentationTiming: vsync policy and FPS labels");
  testTrue(g,
           isVsyncRequested(nullptr),
           "missing environment defaults to synchronized presentation");

  EnvVars env;
  testTrue(g,
           isVsyncRequested(&env),
           "missing vsync value defaults to synchronized presentation");
  env.setVar("vsync", false);
  testTrue(
    g, !isVsyncRequested(&env), "disabled vsync selects uncapped presentation");
  env.setVar("vsync", true);
  testTrue(g,
           isVsyncRequested(&env),
           "enabled vsync selects frame-paced presentation");

  testTrue(g,
           buildFrameRateLabel(true, 144, 144) ==
             "Paced FPS: 144 | Submit FPS: 144",
           "paced label separates swap cadence from submissions");
  testTrue(g,
           buildFrameRateLabel(false, 999, 4812) ==
             "Paced FPS: off | Submit FPS: 4812",
           "uncapped label does not misreport submissions as presented FPS");
}

static void
testAssetManagerEnrollment()
{
  testSection("AssetManager: enroll assets and track counts");
  HeadlessCanvasFixture fixture(4, 4);
  AssetManager assets(&fixture.renderer);
  const size_t createsAfterCanvas = fixture.mock.getCreateCount();
  const std::vector<float> vertices = { 0.0f, 1.0f, 2.0f };
  const unsigned char pixels[] = { 255, 0, 0, 255 };
  ShaderPaths paths;
  paths.vertexPath = "vertex.glsl";
  paths.fragmentPath = "fragment.glsl";
  ShaderSources sources;
  sources.vertexSource = "void main(){}";
  sources.fragmentSource = "void main(){}";

  testEqInt(g,
            static_cast<int>(assets.LoadMeshToGlobal("global.mesh")),
            0,
            "global mesh path gets first id");
  testEqInt(g,
            static_cast<int>(assets.LoadMeshToScene("scene.mesh")),
            1,
            "scene mesh path gets second id");
  testEqInt(g,
            static_cast<int>(assets.LoadMeshToGlobal(vertices)),
            2,
            "global memory mesh gets third id");
  testEqInt(g,
            static_cast<int>(assets.LoadMeshToScene(vertices)),
            3,
            "scene memory mesh gets fourth id");
  testEqInt(g,
            static_cast<int>(assets.GetMeshCount()),
            4,
            "mesh count tracks all enrollments");

  testEqInt(g,
            static_cast<int>(assets.LoadTextureToGlobal("global.png")),
            0,
            "global texture path gets first id");
  testEqInt(g,
            static_cast<int>(assets.LoadTextureToScene("scene.png")),
            1,
            "scene texture path gets second id");
  testEqInt(g,
            static_cast<int>(assets.LoadTextureToGlobal(pixels, 1, 1)),
            2,
            "global memory texture gets third id");
  testEqInt(g,
            static_cast<int>(assets.LoadTextureToScene(pixels, 1, 1)),
            3,
            "scene memory texture gets fourth id");
  testEqInt(g,
            static_cast<int>(assets.GetTextureCount()),
            4,
            "texture count tracks all enrollments");

  testEqInt(g,
            static_cast<int>(assets.LoadShaderToGlobal(paths)),
            0,
            "global shader paths get first id");
  testEqInt(g,
            static_cast<int>(assets.LoadShaderToScene(paths)),
            1,
            "scene shader paths get second id");
  testEqInt(g,
            static_cast<int>(assets.LoadShaderToGlobal(sources)),
            2,
            "global shader sources get third id");
  testEqInt(g,
            static_cast<int>(assets.LoadShaderToScene(sources)),
            3,
            "scene shader sources get fourth id");
  testEqInt(g,
            static_cast<int>(assets.GetShaderCount()),
            4,
            "shader count tracks all enrollments");
  // 4 mesh + 4 texture + 4 shader = 12 backend creates after canvas baseline.
  testEqSize(g,
             fixture.mock.getCreateCount() - createsAfterCanvas,
             12,
             "renderer receives every asset enrollment after canvas setup");
}

static void
testAssetManagerLookupAndShutdown()
{
  testSection("AssetManager: lookup misses, frees, and shutdown");
  HeadlessCanvasFixture fixture(4, 4);
  AssetManager assets(&fixture.renderer);
  assets.LoadMeshToGlobal("mesh");
  assets.LoadTextureToGlobal("texture");
  ShaderSources sources;
  sources.vertexSource = "vertex";
  sources.fragmentSource = "fragment";
  assets.LoadShaderToGlobal(sources);
  testTrue(
    g, assets.GetMesh(99) == nullptr, "missing mesh lookup returns null");
  testTrue(
    g, assets.GetTexture(99) == nullptr, "missing texture lookup returns null");
  testTrue(
    g, assets.GetShader(99) == nullptr, "missing shader lookup returns null");
  testTrue(g, assets.FreeMesh(99), "null mesh lookup entry can be removed");
  testTrue(
    g, assets.FreeTexture(99), "null texture lookup entry can be removed");
  testTrue(g, assets.FreeShader(99), "null shader lookup entry can be removed");
  testTrue(g, !assets.FreeMesh(99), "removed mesh entry is then absent");
  testTrue(g, !assets.FreeTexture(99), "removed texture entry is then absent");
  testTrue(g, !assets.FreeShader(99), "removed shader entry is then absent");
  assets.Shutdown();
  testEqInt(g,
            static_cast<int>(assets.GetMeshCount()),
            0,
            "shutdown resets mesh count");
  testEqInt(g,
            static_cast<int>(assets.GetTextureCount()),
            0,
            "shutdown resets texture count");
  testEqInt(g,
            static_cast<int>(assets.GetShaderCount()),
            0,
            "shutdown resets shader count");
}

static void
testSystemArgumentValidators()
{
  testSection("SysCmdLine: token validators");
  char digits[] = "123456";
  char empty[] = "";
  char mixed[] = "12x";
  char mode[] = "GAME_OF_LIFE";
  char lowerMode[] = "seeds";
  char invalidMode[] = "RULE-90";
  testTrue(g, SysCmdLine::StringIsDigit(digits), "decimal digits accepted");
  testTrue(g,
           SysCmdLine::StringIsDigit(empty),
           "empty token preserves existing validator behavior");
  testTrue(
    g, !SysCmdLine::StringIsDigit(mixed), "mixed numeric token rejected");
  testTrue(g, SysCmdLine::StringisModeString(mode), "underscore mode accepted");
  testTrue(g,
           SysCmdLine::StringisModeString(lowerMode),
           "alphabetic lowercase mode accepted");
  testTrue(g,
           !SysCmdLine::StringisModeString(invalidMode),
           "punctuated mode rejected");
}

static void
testSystemArgumentParsing()
{
  testSection("SysCmdLine: reordered dimension options");
  EnvVars env;
  char executable[] = "illumo";
  char canvasWidth[] = "120";
  char ch[] = "-ch";
  char canvasHeight[] = "90";
  char ww[] = "-ww";
  char width[] = "1024";
  char cw[] = "-cw";
  char wh[] = "-wh";
  char height[] = "768";
  char* arguments[] = { executable, ch,          canvasHeight, ww,    width,
                        cw,         canvasWidth, wh,           height };
  SysCmdLine::ParseCommandLine(9, arguments, &env);
  testTrue(g, env.getVar("WinX").value == "1024", "window width parsed");
  testTrue(g, env.getVar("WinY").value == "768", "window height parsed");
  testTrue(g, env.getVar("CanvasX").value == "120", "canvas width parsed");
  testTrue(g, env.getVar("CanvasY").value == "90", "canvas height parsed");
}

static void
testContextRequirementChecks()
{
  testSection("IllumoContext: required service checks");
  NullRenderWindow window(320, 240);
  EnvVars env;
  Camera camera(glm::vec2(1.0f, 1.0f), 1.0f, &env);
  MockBackend mock;
  mock.Initialize();
  Renderer renderer(&window, &env, &camera, &mock, false);
  CommandRegistry registry;
  CommandLine console(&env, &registry, &window, &renderer);
  InputManager input(nullptr);
  Scene scene(&window, &camera);
  IllumoContext context{ &scene,  &window, &console, &input,   &renderer,
                         nullptr, &env,    &camera,  &registry };
  testTrue(g,
           IllumoContextHasGameCore(&context),
           "game core accepts the interface-based window fixture");
  testTrue(g,
           IllumoContextHasDebugCore(&context),
           "debug core accepts complete services");
  context.scene = nullptr;
  testTrue(g, !IllumoContextHasGameCore(&context), "game core requires scene");
  testTrue(g,
           IllumoContextHasDebugCore(&context),
           "debug core does not require scene");
  context.commandLine = nullptr;
  testTrue(g,
           !IllumoContextHasDebugCore(&context),
           "debug core requires command line");
  testTrue(g, !IllumoContextHasGameCore(nullptr), "null context rejected");
}

static void
testSplashWakeAndTokens()
{
  testSection("SplashText: wake and command emission");
  HeadlessCanvasFixture fixture(4, 4, 320, 240);
  GLString::setRenderWindow(&fixture.window);
  SplashText splash("EDIT", 255, 230, 120, 255, 24, 8, 24, &fixture.renderer);
  testTrue(g, !splash.isVisible(), "splash starts hidden");
  testTrue(g,
           splash.AppendCommands(&fixture.renderer),
           "hidden splash stays on token path");
  testEqSize(g,
             fixture.mock.getPendingCommandCount(),
             0,
             "hidden splash emits no commands");
  splash.Wake();
  testTrue(g, splash.isVisible(), "Wake makes splash visible");
  testTrue(g,
           splash.AppendCommands(&fixture.renderer),
           "awake splash emits through GLString token path");
  testTrue(g,
           fixture.mock.getPendingCommandCount() > 0,
           "awake splash emits render commands");
}

static void
testSplashFadeCompletion()
{
  testSection("SplashText: fade completion");
  SplashText splash;
  splash.Wake();
  std::this_thread::sleep_for(std::chrono::milliseconds(1600));
  splash.Fade();
  testTrue(g, !splash.isVisible(), "splash hides after wake duration");
}

static int
runRuntimeUtilityCase(void (*testFunction)())
{
  g.failures = 0;
  testFunction();
  return g.failures;
}

void
registerRuntimeUtilityTests(IllumoTestRegistry& registry)
{
  registry.add("Illumo.InputContext.Bindings", []() {
    return runRuntimeUtilityCase(testInputContextBindings);
  });
  registry.add("Illumo.InputManager.KeyMappings", []() {
    return runRuntimeUtilityCase(testInputManagerMappings);
  });
  registry.add("Illumo.InputManager.HeadlessLifecycle", []() {
    return runRuntimeUtilityCase(testInputManagerHeadlessLifecycle);
  });
  registry.add("Illumo.InputManager.ContextCapacity", []() {
    return runRuntimeUtilityCase(testInputManagerContextsAndCapacity);
  });
  registry.add("Illumo.BackendConfig.TokenConversion",
               []() { return runRuntimeUtilityCase(testBackendConfigTokens); });
  registry.add("Illumo.Presentation.FramePacingPolicy", []() {
    return runRuntimeUtilityCase(testPresentationTimingPolicy);
  });
  registry.add("Illumo.AssetManager.Enrollment", []() {
    return runRuntimeUtilityCase(testAssetManagerEnrollment);
  });
  registry.add("Illumo.AssetManager.LookupAndShutdown", []() {
    return runRuntimeUtilityCase(testAssetManagerLookupAndShutdown);
  });
  registry.add("Illumo.SysCmdLine.Validators", []() {
    return runRuntimeUtilityCase(testSystemArgumentValidators);
  });
  registry.add("Illumo.SysCmdLine.DimensionOptions", []() {
    return runRuntimeUtilityCase(testSystemArgumentParsing);
  });
  registry.add("Illumo.IllumoContext.RequiredServices", []() {
    return runRuntimeUtilityCase(testContextRequirementChecks);
  });
  registry.add("Illumo.SplashText.WakeAndTokens",
               []() { return runRuntimeUtilityCase(testSplashWakeAndTokens); });
  registry.add("Illumo.SplashText.FadeCompletion", []() {
    return runRuntimeUtilityCase(testSplashFadeCompletion);
  });
}
