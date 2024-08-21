#include "RenderWindow.h"


RenderWindow::RenderWindow(const int width, const int height, const std::string& title, const CellCanvas &canvas) {
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

	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);

	const char* vptr = vertShaderSource.data();
	glShaderSource(vertexShader, 1, &vptr, nullptr);
	glCompileShader(vertexShader);

	if (!getShaderCompileStatus(vertexShader)) std::exit(-1);

	unsigned int fragShader;
	fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fptr = fragShaderSource.data();
	glShaderSource(fragShader, 1, &fptr, nullptr);
	glCompileShader(fragShader);

	if (!getShaderCompileStatus(fragShader)) std::exit(-1);

	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragShader);
	glLinkProgram(shaderProgram);
	glDeleteShader(vertexShader);
	glDeleteShader(fragShader);

	//Setup life canvas texture
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, canvas.getDimensions()[0], canvas.getDimensions()[1], 0, GL_RGB, GL_UNSIGNED_BYTE, &lifeCanvas[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	static unsigned int vert_VBO;
	static unsigned int ebo;
	static unsigned int vao;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vert_VBO);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vert_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);

	GLint posAttrib = glGetAttribLocation(shaderProgram, "aPos");

	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
	glEnableVertexAttribArray(posAttrib);

	GLint texAttrib = glGetAttribLocation(shaderProgram, "aTexCoord");

	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(texAttrib);
}

bool RenderWindow::getShaderCompileStatus(const int shaderProgram)
{
	int success;
	std::string infoLog(512, ' ');
	glGetShaderiv(shaderProgram, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shaderProgram, 512, nullptr, &infoLog[0]);
		std::cerr << "Error: shader compilation failed!\n\n" << infoLog << std::endl;
		return false;
	}
	else return true;
}

void RenderWindow::bindKeyCallback(static std::function<void(GLFWwindow*, const int, int, const int, const int)> func)
{
	glfwSetKeyCallback(window, func.target<void(GLFWwindow*, const int, int, const int, const int)>()); //pain...
}
