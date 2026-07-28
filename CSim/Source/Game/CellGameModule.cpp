#include "CellGameModule.h"
#include <fstream>
#include <algorithm>
#include "Services/Logger.h"
#include "Rendering/SplashText.h"

SplashText* stateSplash = nullptr;

CellGameModule::CellGameModule()
	: cellContext(nullptr)
	, currentState(CellState::EDIT)
	, simAccum(0.0)
	, simStepSeconds(1.0 / 30.0)
{

}

CellGameModule::~CellGameModule()
{
	
}

void CellGameModule::Start(IllumoContext* context)
{
	ic = context;

	// Prefer ModeString from envvars / previous console command.
	std::string startMode = ic->envVars->getVar("ModeString").value;
	if (startMode.empty())
	{
		startMode = "GAME_OF_LIFE";
	}
	this->cellContext = new CellContext(startMode, ic->envVars, ic->window, ic->camera, ic->renderer);

	// Simulation step rate comes from env (tps * speedFactor). Re-read live in Normal().
	simAccum = 0.0;
	syncSimRateFromEnv();

	InputEvent ac;
	ac.keyCode = KeyCode::MouseMiddle;
	ac.inputAction = InputAction::Press;
	this->inputContext.bindAction("CameraPan", ac);

	ac.keyCode = KeyCode::MouseRight;
	ac.inputAction = InputAction::Press;
	this->inputContext.bindAction("CameraRotate", ac);

	ac.keyCode = KeyCode::C;
	ac.inputAction = InputAction::Press;
	this->inputContext.bindAction("ClearCanvas", ac);

	ac.keyCode = KeyCode::E;
	ac.inputAction = InputAction::Press;
	this->inputContext.bindAction("ToggleState", ac);

	ac.keyCode = KeyCode::MouseLeft;
	ac.inputAction = InputAction::Press;
	this->inputContext.bindAction("PaintCanvas", ac);

	long contextId = ic->inputManager->registerInputContext(this->inputContext);
	ic->inputManager->setActiveInputContext(contextId);

	currentState = CellState::EDIT;
	cellContext->getCellCanvas()->setCanvasPixel(50, 40, 0);
	cellContext->getCellCanvas()->setCanvasPixel(51, 40, 0);
	cellContext->getCellCanvas()->setCanvasPixel(52, 40, 0);
	cellContext->getCellCanvas()->setCanvasPixel(52, 39, 0);
	cellContext->getCellCanvas()->setCanvasPixel(51, 38, 0);

	// Initial palette from active ruleset; glider cells already mark R8 upload dirty.
	cellContext->getCellCanvas()->rebuildPalette(cellContext->getRuleSet());
	updateVisualTargets();

	// Mode splash label (top-left corner). Shown briefly when toggling EDIT/NORMAL with E.
	// GLString::setRenderWindow is already set in Illumo::Init before modules Start.
	if (stateSplash == nullptr && ic->renderer != nullptr)
	{
		// Soft yellow, large enough to notice without covering the canvas.
		stateSplash = new SplashText(
			"EDIT",
			255, 230, 120, 255,
			32,
			16, 48,
			ic->renderer);
		stateSplash->setVisible(false);
	}
}

void CellGameModule::updateVisualTargets()
{
	ZoneScopedN("Visual.updateTargets");
	Canvas* canvas = cellContext->getCellCanvas();
	// R8 path: lifeCanvas *is* the GPU source. Paint/generation already expand the
	// upload dirty rect; this just clears the logical dirty flag after a change.
	if (!canvas->isCellsDirty())
	{
		return;
	}
	canvas->onTargetsRebuilt();
}

void CellGameModule::syncSimRateFromEnv()
{
	// Effective rate = tps * speedFactor (both live from env vars / console).
	long tps = ic->envVars->getVar("tps").valueAsLong;
	if (tps < 1)
	{
		tps = 1;
	}
	if (tps > 1000)
	{
		tps = 1000;
	}

	double speedFactor = ic->envVars->getVar("speedFactor").valueAsDouble;
	if (speedFactor <= 0.0)
	{
		speedFactor = 1.0;
	}
	if (speedFactor > 100.0)
	{
		speedFactor = 100.0;
	}

	const double effectiveTps = static_cast<double>(tps) * speedFactor;
	simStepSeconds = 1.0 / effectiveTps;

	// cellFadeSpeed kept for env/console compatibility; R8 palette path snaps colors.
	float fadeSpeed = 8.0f;
	if (ic->envVars->getVar("cellFadeSpeed").value != "")
	{
		fadeSpeed = static_cast<float>(ic->envVars->getVar("cellFadeSpeed").valueAsDouble);
	}
	if (fadeSpeed < 0.0f)
	{
		fadeSpeed = 0.0f;
	}
	cellContext->getCellCanvas()->setFadeSpeed(fadeSpeed);
}

void CellGameModule::Update(double dt)
{
	ZoneNamed(CellGameModuleUpdateZone, "CellGameModule Update");

	// Apply ruleset changes from console (`ruleset SEEDS`) or env ModeString.
	{
		std::string wanted = ic->envVars->getVar("ModeString").value;
		if (!wanted.empty() && wanted != cellContext->getModeString())
		{
			if (cellContext->setRuleSet(wanted))
			{
				std::string msg = "Active ruleset: " + cellContext->getModeString();
				Logger::LogInfo(msg.c_str());
				// Same life values, new colors → rebuild palette only (no cell re-upload).
				cellContext->getCellCanvas()->rebuildPalette(cellContext->getRuleSet());
			}
		}
	}

	// Common behavior: camera panning
	CameraPan();

	// Zoom behavior using scroll offset
	std::array<double, 2> mouseCoords = ic->window->getMouseCoords();
	glm::vec2 worldMouse = ic->camera->ScreenToWorld(glm::vec2(mouseCoords[0], mouseCoords[1]));
	double* scroll = ic->inputManager->getMouseScrollOffset();
	if (*scroll != 0.0f)
	{
		double zoomFactor = (*scroll > 0.0f) ? 1.15 : 0.85;
		ic->camera->ZoomAt(static_cast<float>(zoomFactor), worldMouse);
	}

	// Toggle between NORMAL and EDIT states with 'E' key
	if (ic->inputManager->isActionActive("ToggleState"))
	{
		if (currentState == CellState::NORMAL)
		{
			currentState = CellState::EDIT;
			if (stateSplash)
			{
				stateSplash->setContent("EDIT");
				stateSplash->Wake();
			}
			Logger::LogInfo("State changed to EDIT");
		}
		else
		{
			currentState = CellState::NORMAL;
			simAccum = 0.0;
			if (stateSplash)
			{
				stateSplash->setContent("NORMAL");
				stateSplash->Wake();
			}
			Logger::LogInfo("State changed to NORMAL");
		}
	}

	// State dependent behavior
	switch (currentState)
	{
		case CellState::NORMAL:
			Normal(dt);
			break;
		case CellState::EDIT:
			Edit(dt);
			break;
		case CellState::SAVE:
			break;
		case CellState::LOAD:
			break;
		case CellState::EXIT:
			Exit();
			break;
		default:
			break;
	}

	// R8: clear cellsDirty after paint/sim (upload rect already expanded).
	updateVisualTargets();
	// tickVisual is a no-op on the palette path (kept for API stability).
	cellContext->getCellCanvas()->tickVisual(static_cast<float>(dt));
}

void CellGameModule::Exit()
{
	if (stateSplash != nullptr)
	{
		delete stateSplash;
		stateSplash = nullptr;
	}
	delete cellContext;
	cellContext = nullptr;
}

void CellGameModule::Normal(double dt)
{
	int width = this->cellContext->getCellCanvas()->canvasWidth;
	int height = this->cellContext->getCellCanvas()->canvasHeight;

	// Pick up tps / speedFactor changes from envvars.json or console (`tps 60`).
	syncSimRateFromEnv();

	// Advance simulation on the tps clock, not every render frame.
	if (dt < 0.0)
	{
		dt = 0.0;
	}
	// Avoid huge single-frame jumps after a breakpoint / alt-tab.
	if (dt > 0.25)
	{
		dt = 0.25;
	}

	simAccum += dt;

	// Allow enough steps to honor high tps; still cap so a stall can't melt the CPU.
	const int maxSteps = 64;
	int steps = 0;
	while (simAccum >= simStepSeconds && steps < maxSteps)
	{
		this->cellContext->getRuleSet()->calcGeneration(0, 0, width, height);
		simAccum -= simStepSeconds;
		steps += 1;
	}

	// Drop leftover debt if we hit the cap so we don't forever "catch up".
	if (steps >= maxSteps && simAccum > simStepSeconds)
	{
		simAccum = 0.0;
	}
}

void CellGameModule::Edit(double dt)
{
	(void)dt;
	int width = cellContext->getCellCanvas()->canvasWidth;
	int height = cellContext->getCellCanvas()->canvasHeight;
	static bool wasPressed = false;
	static int lastMouseX = -1;
	static int lastMouseY = -1;

	std::array<double, 2> mouseCoords = ic->window->getMouseCoords();
	glm::vec2 worldPos = ic->camera->ScreenToWorld(glm::vec2(mouseCoords[0], mouseCoords[1]));

	float cellSize = 16.0f;
	int currentX = static_cast<int>(std::floor(worldPos.x / cellSize));
	int currentY = static_cast<int>(std::floor(worldPos.y / cellSize));

	if (!ic->commandLine->isOpen)
	{
		bool isLeftPressed = ic->inputManager->isMouseButtonPressed(KeyCode::MouseLeft);
		bool isRightPressed = ic->inputManager->isMouseButtonPressed(KeyCode::MouseRight);

		if (isLeftPressed || isRightPressed)
		{
			unsigned char colorVal = isLeftPressed ? 0 : 1; // 0 = CELL_ALIVE, 1 = CELL_DEAD

			if (wasPressed)
			{
				int x0 = lastMouseX;
				int y0 = lastMouseY;
				int x1 = currentX;
				int y1 = currentY;
				int dx = abs(x1 - x0);
				int dy = abs(y1 - y0);
				int sx = (x0 < x1) ? 1 : -1;
				int sy = (y0 < y1) ? 1 : -1;
				int err = dx - dy;

				while (true)
				{
					if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height)
					{
						this->cellContext->getCellCanvas()->setCanvasPixel(x0, y0, colorVal);
					}
					if (x0 == x1 && y0 == y1) break;
					int e2 = 2 * err;
					if (e2 > -dy)
					{
						err -= dy;
						x0 += sx;
					}
					if (e2 < dx)
					{
						err += dx;
						y0 += sy;
					}
				}
			}
			else
			{
				if (currentX >= 0 && currentX < width && currentY >= 0 && currentY < height)
				{
					this->cellContext->getCellCanvas()->setCanvasPixel(currentX, currentY, colorVal);
				}
			}
			wasPressed = true;
			lastMouseX = currentX;
			lastMouseY = currentY;
		}
		else
		{
			wasPressed = false;
			if (ic->inputManager->isKeyPressed(KeyCode::C))
				this->cellContext->getCellCanvas()->clearCanvas();
		}
	}
	else
	{
		wasPressed = false;
	}

}

void CellGameModule::SaveCellGame(std::string filename)
{
	if (filename.empty())
	{
		Logger::LogWarning("Save cancelled: file path is empty.");
		return;
	}

	std::fstream myfile(filename, std::ios::binary | std::ios::out);
	if (!myfile.is_open())
	{
		std::string err = "Failed to open file for saving: " + std::string(filename);
		Logger::LogError(err.c_str());
		return;
	}

	myfile.write(this->cellContext->getRuleSet()->getRuleTag().c_str(), MAX_RULETAG_SIZE);
	int width = this->cellContext->getCellCanvas()->canvasWidth;
	int height = this->cellContext->getCellCanvas()->canvasHeight;
	myfile.write(reinterpret_cast<char*>(&width), sizeof(width));
	myfile.write(reinterpret_cast<char*>(&height), sizeof(height));
	myfile.write(reinterpret_cast<char*>(&(this->cellContext->getCellCanvas()->lifeCanvas[0])), width * height);
	myfile.close();

	std::string info = "Saved canvas to File : " + std::string(filename);
	Logger::LogInfo(info.c_str());
}

void CellGameModule::LoadCellGame(std::string filename)
{
	if (filename.empty())
	{
		Logger::LogWarning("Load cancelled: file path is empty.");
		return;
	}

	std::fstream myfile(filename, std::ios::binary | std::ios::in);
	if (!myfile.is_open())
	{
		std::string err = "Failed to open file for loading: " + std::string(filename);
		Logger::LogError(err.c_str());
		return;
	}

	char instring[MAX_RULETAG_SIZE + 1];
	memset(instring, 0, MAX_RULETAG_SIZE + 1);
	static int fWidth = 0;
	static int fHeight = 0;

	myfile.read(instring, MAX_RULETAG_SIZE);
	std::string ruleString = instring;
	if (ruleString != cellContext->getRuleSet()->getRuleTag())
	{
		std::string err = "This data is meant for ruleset: " + ruleString;
		Logger::LogError(err.c_str());
		return;
	}

	myfile.read(reinterpret_cast<char*>(&fWidth), sizeof(fWidth));
	myfile.read(reinterpret_cast<char*>(&fHeight), sizeof(fHeight));
	int width = this->cellContext->getCellCanvas()->canvasWidth;
	int height = this->cellContext->getCellCanvas()->canvasHeight;
	if (fWidth > width || fHeight > height)
	{
		Logger::LogWarning("Input canvas is larger than allocated canvas. Cell data may not read correctly...");
	}

	// Clear the current canvas first to avoid remnants
	memset(this->cellContext->getCellCanvas()->lifeCanvas, 1, width * height);

	// Read only as many bytes as can fit in the current canvas
	int bytesToRead = std::min(fWidth * fHeight, width * height);
	myfile.read(reinterpret_cast<char*>(&(this->cellContext->getCellCanvas()->lifeCanvas[0])), bytesToRead);
	myfile.close();

	std::string info = "Loaded canvas from File : " + std::string(filename);
	Logger::LogInfo(info.c_str());
}

void CellGameModule::CameraPan()
{
	std::array<double, 2> mousePos = ic->inputManager->getMousePosition();
	glm::vec2 worldMouse = ic->camera->ScreenToWorld(glm::vec2(mousePos[0], mousePos[1]));
	static glm::vec2 lastMousePos = worldMouse;
	static bool wasPressed = false;

	if (ic->inputManager->isMouseButtonPressed(KeyCode::MouseMiddle))
	{
		if (!wasPressed)
		{
			lastMousePos = worldMouse;
			wasPressed = true;
		}
		glm::vec2 delta = lastMousePos - worldMouse;
		ic->camera->Pan(delta * ic->camera->GetZoom());
		worldMouse = ic->camera->ScreenToWorld(glm::vec2(mousePos[0], mousePos[1]));
	}
	else
	{
		wasPressed = false;
	}
	lastMousePos = worldMouse;
}

void CellGameModule::CameraRotate()
{

}

void CellGameModule::DispatchDrawables(Scene* scene)
{
	scene->AddDrawable(this->cellContext->getCellCanvas());
	// Mode splash sits above the canvas (token UI path via SplashText::AppendCommands).
	if (stateSplash != nullptr && stateSplash->isVisible())
	{
		scene->AddDrawable(stateSplash);
	}
}