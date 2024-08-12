#include <Windows.h>
#include <wingdi.h>
#include <glew.h>
#include <gl/GL.h>
#include <glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <thread>
using namespace std;
/*
Two triangles to cover the screen so I can cover it with a texture

yes, i can do it with one triangle but i dont feel like figuring that out

*/

enum application_mode {
	NORMAL, EDIT
};

const int canvasX = 80;
const int canvasY = 60;
float speedFactor = 50.0f;
int windowX = 1280;
int windowY = 720;
const float speedFactorMin = 1.0f;
const float speedFactorMax = 250.0f;
bool isPaused = false;
bool isFullScreen = false;
bool start_sim = false;
application_mode currentMode = application_mode::NORMAL;
double mousePosX;
double mousePosY;

float vertices[] = {
	// positions          // colors           // texture coords
	 1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
	 1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
	-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
	-1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
};
unsigned int indices[] = {
	0, 1, 3, // first triangle
	1, 2, 3  // second triangle
};

void setCanvasCell(vector<GLubyte> &canvas, int x, int y, int val) {
	if (x > canvasX - 1 || y > canvasY - 1) {
		cerr << "out of canvas!" << endl;
		return;
	}
	y = (canvasY-1) - y; //invert the y axis
	canvas[(canvasX*3*y) + (3 * x) + 0] = 255*val;
	canvas[(canvasX*3*y) + (3 * x) + 1] = 255*val;
	canvas[(canvasX*3*y) + (3 * x) + 2] = 255*val;
}

int getCanvasCell(vector<GLubyte> &canvas, int x, int y) {
	if (x > canvasX - 1 || y > canvasY - 1) {
		cerr << "out of canvas!" << endl;
		return -1;
	}
	y = (canvasY - 1) - y; //invert the y axis

	return canvas[(canvasX * 3 * y) + (3 * x) + 1] == 0;
}

int count_neighbor(int r, const int c, const int w, const int h, vector<GLubyte>& canvas) {
	int i, j, i2, j2 = 0;
	int count = 0;
	//r = canvasY - r;
	for (i = r - 1; i <= r + 1; i++) {
		for (j = c - 1; j <= c + 1; j++) {
			i2 = i;
			j2 = j;
			if (i == r && j == c) continue;
			if (i < 0)
			{
				i2 = w - 1;

			}
			if (j < 0) {
				j2 = h - 1;
			}
			if (i >= w) {
				i2 = 0;
			}
			if (j >= h) {
				j2 = 0;
			}
			if (getCanvasCell(canvas, i2, j2)) {
				count++;
			}
		}
	}
	return count;
}

void calc_generation(const int x_start, const int y_start, const int x_end, const int y_end, vector<GLubyte> &canvas) {

	int canvasVectorWidth = x_end - x_start;
	int canvasVectorHeight = y_end - y_start;

	vector<vector<int>> ne(canvasVectorWidth, vector<int>(canvasVectorHeight));
	for (int i = 0; i < ne.size(); i++) {
		fill(ne[i].begin(), ne[i].end(), 0);
	}
	
	for (int i = 0; i < canvasVectorWidth; i++) {
		for (int j = 0; j < canvasVectorHeight; j++) {
			ne[i][j] = count_neighbor(i, j, canvasVectorWidth, canvasVectorHeight, canvas);
		}
	}

	for (int i = 0; i < canvasVectorWidth; i++) {
		for (int j = 0; j < canvasVectorHeight; j++) {
			if (getCanvasCell(canvas, i, j) && (ne[i][j] == 2 || ne[i][j] == 3)) {
				setCanvasCell(canvas, i, j, 0);
			}
			else if (!getCanvasCell(canvas, i, j) && ne[i][j] == 3) {
				setCanvasCell(canvas, i, j, 0);
			}
			else {
				setCanvasCell(canvas, i, j, 1);
			}
		}
	}
	
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS && currentMode == application_mode::EDIT) {

	}
}


bool getShaderCompileStatus(int shaderProgram, string name) {
	int success;
	char infoLog[512];
	glGetShaderiv(shaderProgram, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shaderProgram, 512, nullptr, infoLog);
		cerr << "Error:" << name << " shader compilation failed!\n\n" << infoLog << endl;
		return false;
	}
	else return true;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	if (key == GLFW_KEY_S && action == GLFW_PRESS) {
		start_sim = true;
	}
	if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
		if (speedFactor >= speedFactorMin) speedFactor /= 1.5f;
		else speedFactor = speedFactorMin;
		cout << "changed delay to: " << speedFactor << endl;
	}
	if (key == GLFW_KEY_MINUS && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT) {
		if (speedFactor <= speedFactorMax)speedFactor *= 1.5f;
		else speedFactor = speedFactorMax;
		
		cout << "changed delay to: " << speedFactor << endl;
	}
	if (key == GLFW_KEY_P && action == GLFW_PRESS) {
		isPaused = !isPaused;
		
		if (isPaused) cout << "PAUSE" << endl;
		else cout << "RESUME" << endl;
	}
	if (key == GLFW_KEY_F11 && action == GLFW_PRESS) {
		if (!isFullScreen) {
			glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, 1280, 720, 144);
		}
		else {
			glfwSetWindowMonitor(window, nullptr, 1280/2, 720/2, 1280, 720, 144);
		}
		isFullScreen = !isFullScreen;
	}
	if (key == GLFW_KEY_E && action == GLFW_PRESS) {
		if (currentMode == application_mode::NORMAL)
		{
			currentMode = application_mode::EDIT;
			cout << "EDIT" << endl;
		}
		else {
			currentMode = application_mode::NORMAL;
			cout << "NORMAL" << endl;
		}
	}
}


int main(void)
{
	vector<GLubyte> lifeCanvas(canvasX * canvasY * 3, 0);
	fill(lifeCanvas.begin(), lifeCanvas.end(), 255);
    /* Initialize the library */
    if (!glfwInit())
        return -1;


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    /* Create a windowed mode window and its OpenGL context */
    GLFWwindow* window;
    window = glfwCreateWindow(windowX, windowY, "Game of Life", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
   
	glfwSetKeyCallback(window, key_callback);
    /* Make the window's context current */
    glfwMakeContextCurrent(window);
    glewExperimental = true;
    /* Loop until the user closes the window */
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        cerr << "Error: " << glewGetErrorString(err) << endl;
        return 1;
    }
    const GLubyte* versionGL = glGetString(GL_VERSION);

    cout << versionGL << endl;

	//Load in shader files
	ifstream fragShaderFile;
	fragShaderFile.open("triangle_frag.glsl");

	ifstream vertexShaderFile;
	vertexShaderFile.open("triangle_vertex.glsl");



	string fragShaderSource;
	string vertShaderSource;
	string line;

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

	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);

	const char* vptr = vertShaderSource.data();
	glShaderSource(vertexShader, 1, &vptr, nullptr);
	glCompileShader(vertexShader);

	if (!getShaderCompileStatus(vertexShader, "vertex")) return 1;

	unsigned int fragShader;
	fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fptr = fragShaderSource.data();
	glShaderSource(fragShader, 1, &fptr, nullptr);
	glCompileShader(fragShader);

	if (!getShaderCompileStatus(fragShader, "fragment")) return 1;

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
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, canvasX, canvasY, 0, GL_RGB, GL_UNSIGNED_BYTE, &lifeCanvas[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	unsigned int vert_VBO;
	unsigned int ebo;
	unsigned int vao;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vert_VBO);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vert_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	GLint posAttrib = glGetAttribLocation(shaderProgram, "aPos");

	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(posAttrib);

	GLint texAttrib = glGetAttribLocation(shaderProgram, "aTexCoord");

	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(texAttrib);

	//make a glider for now
	setCanvasCell(lifeCanvas, 50, 40, 0);
	setCanvasCell(lifeCanvas, 51, 40, 0);
	setCanvasCell(lifeCanvas, 52, 40, 0);
	setCanvasCell(lifeCanvas, 52, 39, 0);
	setCanvasCell(lifeCanvas, 51, 38, 0);
	
	float time =  (float) glfwGetTime();
	float delta = 0.0f;

	float printTimeTarget = 2.0f;
	float printTime = 0.0f;
	glfwSwapInterval(0); // Vsync is for bitches

	

    while (!glfwWindowShouldClose(window))
    {
		/* Poll for and process events */
		glfwPollEvents();

		if (!isPaused && start_sim && currentMode == application_mode::NORMAL)
		{ 
			/* Render here */
			//calc simulation generations
			calc_generation(0, 0, canvasX, canvasY, lifeCanvas);
			Sleep(speedFactor);
		}
		else if (!isPaused && start_sim && currentMode == application_mode::EDIT) {
			//allow the user to paint cells onto the screen
			glfwGetCursorPos(window, &mousePosX, &mousePosY);

			int scaledPixelX = (int)(windowX / canvasX);
			int scaledPixelY = (int)(windowY / canvasY);

			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
				setCanvasCell(lifeCanvas, mousePosX/scaledPixelX, mousePosY / scaledPixelY, 0);	
			}
			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)) {
				setCanvasCell(lifeCanvas, mousePosX / scaledPixelX, mousePosY / scaledPixelY, 1);
			}
			if (glfwGetKey(window, GLFW_KEY_C)) fill(lifeCanvas.begin(), lifeCanvas.end(), 255);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, canvasX, canvasY, 0, GL_RGB, GL_UNSIGNED_BYTE, &lifeCanvas[0]);
		glUseProgram(shaderProgram);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		/* Swap front and back buffers */
		glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}