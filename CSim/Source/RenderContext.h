#pragma once

/*
This class will store the contextual information related to rendering the window.
*/

static struct RenderContext {
	unsigned int vertexShader = 0;
	unsigned int fragmaneShader = 0;
	unsigned int shaderProgram = 0;
	unsigned int canvasTextureID = 0;
	unsigned int vbo = 0;
	unsigned int ebo = 0;
	unsigned int vao = 0;
} pipeline;