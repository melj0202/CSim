#include "CellGameModule.h"
#include <fstream>
#include <algorithm>
#include "System/Logger.h"
#include "Rendering/SplashText.h"

SplashText* stateSplash = nullptr;

CellGameModule::CellGameModule()
{

}

CellGameModule::~CellGameModule()
{
	
}

void CellGameModule::Start(IllumoContext* context)
{
	ic = context;

	this->cellContext = new CellContext("GAME_OF_LIFE", ic->envVars, ic->window, ic->camera);

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
}

void CellGameModule::Update(double dt)
{
	ZoneNamed(CellGameModuleUpdateZone, "CellGameModule Update");
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
	if (ic->inputManager->isKeyPressed(KeyCode::E))
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
			Normal();
			break;
		case CellState::EDIT:
			Edit();
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
}

void CellGameModule::Exit()
{
	delete cellContext;
}

void CellGameModule::Normal()
{
	int width = this->cellContext->getCellCanvas()->canvasWidth;
	int height = this->cellContext->getCellCanvas()->canvasHeight;

	this->cellContext->getRuleSet()->calcGeneration(0, 0, width, height);
	for (int i = 0; i < width * height; i++)
	{
		this->cellContext->getRuleSet()->evalCell(this->cellContext->getCellCanvas()->lifeCanvas[i], &(this->cellContext->getCellCanvas()->texCanvasBuffer[i * 3]));
	}
}

void CellGameModule::Edit()
{
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

	for (int i = 0; i < width * height; i++)
	{
		this->cellContext->getRuleSet()->evalCell(this->cellContext->getCellCanvas()->lifeCanvas[i], &(this->cellContext->getCellCanvas()->texCanvasBuffer[i * 3]));
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
}