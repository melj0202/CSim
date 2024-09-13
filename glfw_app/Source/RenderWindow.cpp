#include "RenderWindow.h"


RenderWindow::RenderWindow(const int width, const int height, const std::string& title, const CellCanvas &canvas) : windowWidth(width), windowHeight(height), windowTitle(title) {
	/* Initialize the library */
	if (!glfwInit())
		std::exit(-1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(width, height, "Game of Life", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		std::exit(-1);
	}
	glfwSetWindowPos(window, width / 2, height / 2);

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	glewExperimental = true;
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
	/* Loop until the user closes the window */
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
		std::exit(-1);
	}
	const GLubyte* versionGL = glGetString(GL_VERSION);

	std::cout << versionGL << std::endl;

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

	context.vertexShader = glCreateShader(GL_VERTEX_SHADER);

	const char* vptr = vertShaderSource.data();
	glShaderSource(context.vertexShader, 1, &vptr, nullptr);
	glCompileShader(context.vertexShader);

	if (!getShaderCompileStatus(context.vertexShader)) std::exit(-1);

	context.fragmaneShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fptr = fragShaderSource.data();
	glShaderSource(context.fragmaneShader, 1, &fptr, nullptr);
	glCompileShader(context.fragmaneShader);

	if (!getShaderCompileStatus(context.fragmaneShader)) std::exit(-1);

	context.shaderProgram = glCreateProgram();

	glAttachShader(context.shaderProgram, context.vertexShader);
	glAttachShader(context.shaderProgram, context.fragmaneShader);
	glLinkProgram(context.shaderProgram);
	glDeleteShader(context.vertexShader);
	glDeleteShader(context.fragmaneShader);

	//Setup life canvas texture
	glGenTextures(1, &context.canvasTextureID);
	glBindTexture(GL_TEXTURE_2D, context.canvasTextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, canvas.getDimensions()[0], canvas.getDimensions()[1], 0, GL_RGB, GL_UNSIGNED_BYTE, &canvas.getCanvasState()[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glGenVertexArrays(1, &context.vao);
	glGenBuffers(1, &context.vbo);
	glGenBuffers(1, &context.ebo);

	glBindVertexArray(context.vao);

	glBindBuffer(GL_ARRAY_BUFFER, context.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context.ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);

	GLint posAttrib = glGetAttribLocation(context.shaderProgram, "aPos");

	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
	glEnableVertexAttribArray(posAttrib);

	GLint texAttrib = glGetAttribLocation(context.shaderProgram, "aTexCoord");

	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(texAttrib);

	glfwSwapInterval(0);
}

bool RenderWindow::getShaderCompileStatus(const int shaderProgram) const
{
	int success;
	std::string infoLog(512, ' ');
	glGetShaderiv(shaderProgram, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(context.shaderProgram, 512, nullptr, &infoLog[0]);
		std::cerr << "Error: shader compilation failed!\n\n" << infoLog << std::endl;
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
void RenderWindow::updateWindow(const CellCanvas& canvas) {
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, canvas.getDimensions()[0], canvas.getDimensions()[1], 0, GL_RGB, GL_UNSIGNED_BYTE, &canvas.getCanvasState()[0]);
	glUseProgram(context.shaderProgram);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
	/* Swap front and back buffers */
	glfwSwapBuffers(window);
}

std::array<double, 2> RenderWindow::getMouseCoords()
{
	glfwGetCursorPos(window, &mouseCoords[0], &mouseCoords[1]);
	return mouseCoords;
}
