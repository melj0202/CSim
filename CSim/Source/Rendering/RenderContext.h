#pragma once

/*
This class will store the contextual information related to rendering the window.
*/

static struct RenderContext {
	static unsigned int vertexShader;
	static unsigned int fragmaneShader;
	static unsigned int shaderProgram;
	static unsigned int canvasTextureID;
	static unsigned int vbo;
	static unsigned int ebo;
	static unsigned int vao;
} pipeline;

