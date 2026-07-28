#include "Drawable.h"
#include <iostream>
#include <ostream>
#include <string>
#include <GL/glew.h>

unsigned int DrawableBase::compileShader(unsigned int type, const char* source)
{
	unsigned int id = glCreateShader(type);
	glShaderSource(id, 1, &source, nullptr);
	glCompileShader(id);

	int result;
	glGetShaderiv(id, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE)
	{
		int length;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
		std::string message(length, ' ');
		glGetShaderInfoLog(id, length, &length, &message[0]);
		std::cerr << "Failed to compile shader (" << (type == GL_VERTEX_SHADER ? "vertex" : "fragment") << "): " << message << std::endl;
		glDeleteShader(id);
		return 0;
	}
	return id;
}

unsigned int DrawableBase::createShaderProgram(const char* vertexSource, const char* fragmentSource)
{
	unsigned int program = glCreateProgram();
	unsigned int vs = compileShader(GL_VERTEX_SHADER, vertexSource);
	unsigned int fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);

	int linked = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE)
	{
		int length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
		std::string message(length, ' ');
		glGetProgramInfoLog(program, length, &length, &message[0]);
		std::cerr << "Failed to link shader program: " << message << std::endl;
	}

	glValidateProgram(program);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return program;
}