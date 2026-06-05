//This class represents a string that is drawn with opengl and stb_easy_font
#pragma once

#include <string>
#include <cstdint>
#include "Drawable.h"

struct VertexData {
    float x, y, z;
    uint8_t color[4];
};

class IRenderWindow;

class GLString : public Drawable<GLString> {

private:
	std::string content;
	int r, g, b, a;
	float size_pt;
	int x, y;
    VertexData vertices[2000 * 6];
	
	void initGL();
public:
	static inline IRenderWindow* s_window = nullptr;
	static void setRenderWindow(IRenderWindow* window) { s_window = window; }

	GLString();
	GLString(std::string content, int r, int g, int b, int a, int size_pt, int x, int y);
	~GLString();
	
	void setContent(std::string newContent);
	void setR(int newR);
	void setG(int newG);
	void setB(int newB);
	void setA(int newA);
	void setSize(int newSize);
	void setX(int newX);
	void setY(int newY);
	virtual void DrawImpl();
	
	std::string getContent();
	int getR();
	int getG();
	int getB();
	int getA();
	int getSize();
	int getX();
	int getY();
};