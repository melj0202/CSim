#include "RenderWindow.h"
#include "CellCanvas.h"
#include "RenderContext.h"
#include "../thirdparty/stb/stb_easy_font.h"
#include "CellLogger.h"

int RenderWindow::windowWidth = 1280;
int RenderWindow::windowHeight = 720;
std::array<double, 2> RenderWindow::mouseCoords{ 0.0, 0.0 };
static std::string windowTitle = "CSim";
GLFWwindow* RenderWindow::window = nullptr;
const std::array<float, 32> RenderWindow::vertices = {
	// positions          // colors           // texture coords
		 1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
		 1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
		-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
		-1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
};

const std::array<unsigned char, 32> RenderWindow::indices = {
	1, 2, 3,
	0, 1, 3
};

unsigned int RenderContext::vertexShader = 0;
unsigned int RenderContext::fragmaneShader = 0;
unsigned int RenderContext::shaderProgram = 0;
unsigned int RenderContext::canvasTextureID = 0;
unsigned int RenderContext::vbo = 0;
unsigned int RenderContext::ebo = 0;
unsigned int RenderContext::vao = 0;

RenderWindow::RenderWindow(const int width, const int height, const std::string& title){
	/* Initialize the library */
	if (!glfwInit())
		std::exit(-1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		std::exit(-1);
	}
	glfwSetWindowPos(window, width / 2, height / 2);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glewExperimental = true;
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
	/* Loop until the user closes the window */
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
		std::exit(-1);
	}
	char* versionGL = (char*)glGetString(GL_VERSION);

	char fullGLString[256] = "OpenGL Context: ";

	strcat(fullGLString, versionGL);

	CellLogger::LogInfo(fullGLString);

	//Load in shader files
	std::ifstream fragShaderFile;
	fragShaderFile.open("Shader/triangle_frag.glsl");

	std::ifstream vertexShaderFile;
	vertexShaderFile.open("Shader/triangle_vertex.glsl");

	std::string fragShaderSource;
	std::string vertShaderSource;
	std::string line;

	if (fragShaderFile.is_open())
	{
		while (getline(fragShaderFile, line)) {
			fragShaderSource.append(line);
			fragShaderSource.append("\n");
		}
	}

	if (vertexShaderFile.is_open()) {
		while (getline(vertexShaderFile, line)) {
			vertShaderSource.append(line);
			vertShaderSource.append("\n");
		}
	}

	fragShaderFile.close();
	vertexShaderFile.close();

	pipeline.vertexShader = glCreateShader(GL_VERTEX_SHADER);

	const char* vptr = vertShaderSource.data();
	glShaderSource(pipeline.vertexShader, 1, &vptr, nullptr);
	glCompileShader(pipeline.vertexShader);

	if (!getShaderCompileStatus(pipeline.vertexShader)) std::exit(-1);

	CellLogger::LogInfo("Vertex shader compilation successful...");

	pipeline.fragmaneShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fptr = fragShaderSource.data();
	glShaderSource(pipeline.fragmaneShader, 1, &fptr, nullptr);
	glCompileShader(pipeline.fragmaneShader);

	if (!getShaderCompileStatus(pipeline.fragmaneShader)) std::exit(-1);

	CellLogger::LogInfo("Fragment shader compilation successful...");

	pipeline.shaderProgram = glCreateProgram();

	glAttachShader(pipeline.shaderProgram, pipeline.vertexShader);
	glAttachShader(pipeline.shaderProgram, pipeline.fragmaneShader);
	glLinkProgram(pipeline.shaderProgram);
	CellLogger::LogInfo("Shader Program Linked Successfully...");
	glDeleteShader(pipeline.vertexShader);
	glDeleteShader(pipeline.fragmaneShader);

	//Setup life canvas texture
	glGenTextures(1, &pipeline.canvasTextureID);
	glBindTexture(GL_TEXTURE_2D, pipeline.canvasTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, getDimensions()[0], getDimensions()[1], 0, GL_RGB, GL_UNSIGNED_BYTE, &CellCanvas::lifeCanvas[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glGenVertexArrays(1, &pipeline.vao);
	glGenBuffers(1, &pipeline.vbo);
	glGenBuffers(1, &pipeline.ebo);

	glBindVertexArray(pipeline.vao);

	glBindBuffer(GL_ARRAY_BUFFER, pipeline.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pipeline.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);

	GLuint posAttrib = glGetAttribLocation(pipeline.shaderProgram, "aPos");

	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
	glEnableVertexAttribArray(posAttrib);

	GLuint texAttrib = glGetAttribLocation(pipeline.shaderProgram, "aTexCoord");

	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(texAttrib);

	glfwSwapInterval(0);
}

bool RenderWindow::getShaderCompileStatus(const int shaderProgram)
{
	int success;
	std::string infoLog(512, ' ');
	glGetShaderiv(shaderProgram, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(pipeline.shaderProgram, 512, nullptr, &infoLog[0]);

		CellLogger::LogError("shader compilation failed!\n");
		return false;
	}
	else return true;
}
/*
void RenderWindow::bindKeyCallback(static std::function<void(GLFWwindow*, const int, int, const int, const int)> func)
{
	glfwSetKeyCallback(window, func.target<void(GLFWwindow*, const int, int, const int, const int)>()); //pain...
}
*/
void RenderWindow::updateWindow() {
	//Draw the canvas
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, getDimensions()[0], getDimensions()[1], 0, GL_RGB, GL_UNSIGNED_BYTE, &CellCanvas::texCanvasBuffer[0]);
	glUseProgram(pipeline.shaderProgram);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

	/* Swap front and back buffers */
	glfwSwapBuffers(window);
}

std::array<double, 2> RenderWindow::getMouseCoords()
{
	glfwGetCursorPos(window, &mouseCoords[0], &mouseCoords[1]);
	return mouseCoords;
}

void RenderWindow::centerWindow() {
  //Get monitor dimensions
  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(monitor);

}