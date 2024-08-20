#include <GL/glew.h>
//#include <gl/GL.h>
//#include <glfw3.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>
#include <thread>
#include <array>
#include "Renderer/RenderWindow.h"
#include "MacroDefs.h"
#include <omp.h>
#include "OS/UI/OSMessageBox.h"

#define BLOCK_X 2
#define BLOCK_Y 2

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
const int windowX = 1280;
const int windowY = 720;
const float speedFactorMin = 1.0f;
const float speedFactorMax = 250.0f;
bool isPaused = false;
bool isFullScreen = false;
bool start_sim = false;
application_mode currentMode = application_mode::NORMAL;
double mousePosX;
double mousePosY;
const int scaledPixelX = (windowX / canvasX);
const int scaledPixelY = (windowY / canvasY);
bool wireframe = false;

const std::array<float, 32> vertices = {
	// positions          // colors           // texture coords
	 1.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
	 1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
	-1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
	-1.0f,  1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
};
const std::array<unsigned char, 32> indices = {
	0, 1, 3,
	1, 2, 3

};

__COBALT_FORCE_INLINE__ void setCanvasCell(vector<GLubyte> &canvas, const  int x, const int y, const bool val) {
	//if (x > canvasX - 1 || y > canvasY - 1) return;
	memset(&canvas[canvasX * 3 * (canvasY - 1 - y) + 3 * x], 255 * val, 3);
}


__COBALT_FORCE_INLINE__ void updateCanvas(GLFWwindow* window, const unsigned int shaderProgram, const vector<GLubyte>& canvas) {
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, canvasX, canvasY, 0, GL_RGB, GL_UNSIGNED_BYTE, &canvas[0]);
	glUseProgram(shaderProgram);
	if (wireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
	/* Swap front and back buffers */
	glfwSwapBuffers(window);
}

__COBALT_FORCE_INLINE__ int getCanvasCell(const vector<GLubyte> &canvas, const int x, const int y) {
	//if (x > canvasX - 1 || y > canvasY - 1) return -1;
	return canvas[canvasX * 3 * (canvasY - 1 - y) + 3 * x] == 0;
}

constexpr int count_neighbor(const int r, const int c, const int w, const int h, const vector<GLubyte>& canvas) {
	int i2 = 0;
	int j2 = 0;
	int count = 0;
	for (int i = r - 1; i <= r + 1; i++) {
		for (int j = c - 1; j <= c + 1; j++) {
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

	const int canvasVectorWidth = x_end - x_start;
	const int canvasVectorHeight = y_end - y_start;

	vector<vector<int>> ne(canvasVectorWidth, vector<int>(canvasVectorHeight));
	for (int i = 0; i < ne.size(); i++) {
		fill(ne[i].begin(), ne[i].end(), 0);
	}
	
	for (int j = 0; j < canvasVectorHeight; j += BLOCK_Y) {
		for (int i = 0; i < canvasVectorWidth; i += BLOCK_X) {
			for (int by = 0; by < BLOCK_Y; by++) {
				for (int bx = 0; bx < BLOCK_X; bx++) {
					ne[i + bx][j + by] = count_neighbor(i + bx, j + by, canvasVectorWidth, canvasVectorHeight, canvas);
				}
			}
		}
	}

	for (int i = 0; i < canvasVectorWidth; i+=BLOCK_X) {
		for (int j = 0; j < canvasVectorHeight; j+=BLOCK_Y) {
			for (int by = 0; by < BLOCK_Y; by++) {
				for (int bx = 0; bx < BLOCK_X; bx++) {
					if (getCanvasCell(canvas, i+bx, j+by) && (ne[i + bx][j + by] == 2 || ne[i + bx][j + by] == 3)) {
						setCanvasCell(canvas, i + bx, j + by, 0);
					}
					else if (!getCanvasCell(canvas, i + bx, j + by) && ne[i + bx][j + by] == 3) {
						setCanvasCell(canvas, i + bx, j + by, 0);
					}
					else {
						setCanvasCell(canvas, i + bx, j + by, 1);
					}
				}
			}
			
		}
	}
	
}

bool getShaderCompileStatus(const int shaderProgram) {
	int success;
	std::string infoLog(512, ' ');
	glGetShaderiv(shaderProgram, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(shaderProgram, 512, nullptr, &infoLog[0]);
		cerr << "Error: shader compilation failed!\n\n" << infoLog << endl;
		return false;
	}
	else return true;
}

static void key_callback(GLFWwindow* window, const int key, int, const int action, const int mods)
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
			glfwSetWindowMonitor(window, nullptr, windowX/2, windowY/2, 1280, 720, 144);
		}
		isFullScreen = !isFullScreen;
	}
	if (key == GLFW_KEY_E && action == GLFW_PRESS && start_sim) {
		if (currentMode == application_mode::NORMAL)
		{
			currentMode = application_mode::EDIT;
			glfwSwapInterval(0); // Vsync is for bitches
			cout << "EDIT" << endl;
		}
		else {
			currentMode = application_mode::NORMAL;
			glfwSwapInterval(0); // Vsync is for bitches
			cout << "NORMAL" << endl;
		}
	}
	if (key == GLFW_KEY_F8 && action == GLFW_PRESS) {
		wireframe = !wireframe;
	}
}

int main(void)
{
	vector<GLubyte> lifeCanvas(canvasX * canvasY * 3, 0);
	fill(lifeCanvas.begin(), lifeCanvas.end(), 255);
    /* Initialize the library */
    if (!glfwInit())
        return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
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
	glfwSetWindowPos(window, windowX / 2, windowY / 2);

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
	fragShaderFile.open("Source/Shader/triangle_frag.glsl");

	ifstream vertexShaderFile;
	vertexShaderFile.open("Source/Shader/triangle_vertex.glsl");

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

	fragShaderFile.close();
	vertexShaderFile.close();

	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);

	const char* vptr = vertShaderSource.data();
	glShaderSource(vertexShader, 1, &vptr, nullptr);
	glCompileShader(vertexShader);

	if (!getShaderCompileStatus(vertexShader)) return 1;

	unsigned int fragShader;
	fragShader = glCreateShader(GL_FRAGMENT_SHADER);
	const char* fptr = fragShaderSource.data();
	glShaderSource(fragShader, 1, &fptr, nullptr);
	glCompileShader(fragShader);

	if (!getShaderCompileStatus(fragShader)) return 1;

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

	//make a glider for now
	setCanvasCell(lifeCanvas, 50, 40, 0);
	setCanvasCell(lifeCanvas, 51, 40, 0);
	setCanvasCell(lifeCanvas, 52, 40, 0);
	setCanvasCell(lifeCanvas, 52, 39, 0);
	setCanvasCell(lifeCanvas, 51, 38, 0);
	
	glfwSwapInterval(0); // Vsync is for bitches

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);

	updateCanvas(window, shaderProgram, lifeCanvas);

    while (!glfwWindowShouldClose(window))
    {
		/* Poll for and process events */
		glfwPollEvents();
		if (!isPaused && start_sim && currentMode == application_mode::NORMAL)
		{ 
			/* Render here */
			//calc simulation generations
			calc_generation(0, 0, canvasX, canvasY, lifeCanvas);
			updateCanvas(window, shaderProgram, lifeCanvas);
			__SLEEP(static_cast<DWORD>(speedFactor));
		}
		else if (currentMode == application_mode::EDIT) {
			//allow the user to paint cells onto the screen
			glfwGetCursorPos(window, &mousePosX, &mousePosY);

			if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)) {
				setCanvasCell(lifeCanvas, static_cast<int>(mousePosX/scaledPixelX), static_cast<int>(mousePosY / scaledPixelY), 0);
				updateCanvas(window, shaderProgram, lifeCanvas);
			}
			else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT)) {
				setCanvasCell(lifeCanvas, static_cast<int>(mousePosX / scaledPixelX), static_cast<int>(mousePosY / scaledPixelY), 1);
				updateCanvas(window, shaderProgram, lifeCanvas);
			}
			else if (glfwGetKey(window, GLFW_KEY_C))
			{
				fill(lifeCanvas.begin(), lifeCanvas.end(), 255);
				updateCanvas(window, shaderProgram, lifeCanvas);
			}
		}
		
		
    }

    glfwTerminate();
    return 0;
}