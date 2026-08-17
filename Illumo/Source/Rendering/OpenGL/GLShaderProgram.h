#pragma once
#include "Rendering/IShaderProgram.h"
#include <GL/glew.h> // Or your preferred OpenGL loader header
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

class GLShaderProgram : public IShaderProgram
{
public:
  // Constructor keeps it simple and safe
  GLShaderProgram(const ShaderPaths& paths)
  {
    _programID = 0;
    CompileAndLink(paths);
  }
  GLShaderProgram(const ShaderSources& sources)
  {
    _programID = 0;
    CompileAndLink(sources);
  }

  // RAII Destructor prevents GPU memory leaks
  ~GLShaderProgram()
  {
    // Do nothing. asset destruction should be explicit
  }

  unsigned long GetID() const override { return _programID; }
  bool isValid() const override { return _valid && _programID != 0; }

  void Destroy() override
  {
    if (_programID != 0) {
      glDeleteProgram(_programID);
      _programID = 0;
    }
    _valid = false;
  }

private:
  bool _valid = false;
  std::string ReadFile(const std::string& filePath)
  {
    std::ifstream file(filePath);
    if (!file.is_open()) {
      std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << filePath
                << std::endl;
      return "";
    }
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
  }

  void CompileAndLink(const ShaderSources& sources) override
  {
    CompileAndLink(sources.vertexSource, sources.fragmentSource);
  }

  void CompileAndLink(const ShaderPaths& paths) override
  {
    std::string vertexCode = ReadFile(paths.vertexPath);
    std::string fragmentCode = ReadFile(paths.fragmentPath);
    if (!vertexCode.empty() && !fragmentCode.empty()) {
      CompileAndLink(vertexCode, fragmentCode);
    }
  }

  void CompileAndLink(const std::string& vertexSource,
                      const std::string& fragmentSource) override
  {
    unsigned int vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (vs == 0 || fs == 0) {
      if (vs != 0) {
        glDeleteShader(vs);
      }
      if (fs != 0) {
        glDeleteShader(fs);
      }
      _valid = false;
      return;
    }

    _programID = glCreateProgram();
    glAttachShader(_programID, vs);
    glAttachShader(_programID, fs);
    glLinkProgram(_programID);

    // Check Link Status
    int linked;
    glGetProgramiv(_programID, GL_LINK_STATUS, &linked);
    if (linked == GL_FALSE) {
      int length = 0;
      glGetProgramiv(_programID, GL_INFO_LOG_LENGTH, &length);
      std::vector<char> message(length);
      glGetProgramInfoLog(_programID, length, &length, message.data());
      std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                << message.data() << std::endl;
      glDeleteProgram(_programID);
      _programID = 0;
      _valid = false;
    } else {
      _valid = true;
    }

    // Clean up intermediate shader objects
    glDeleteShader(vs);
    glDeleteShader(fs);
  }

  unsigned int CompileShader(unsigned int type, const std::string& source)
  {
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (result == GL_FALSE) {
      int length;
      glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
      std::vector<char> message(length);
      glGetShaderInfoLog(id, length, &length, message.data());
      std::cerr << "ERROR::SHADER::COMPILATION_FAILED_FOR_"
                << (type == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT") << "\n"
                << message.data() << std::endl;
      glDeleteShader(id);
      return 0;
    }
    return id;
  }
};
