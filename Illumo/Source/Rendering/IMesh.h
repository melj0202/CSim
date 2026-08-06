#pragma once

#include <iostream>
#include <vector>

// Vertex attribute layout for enrolled meshes.
enum class MeshVertexLayout : int
{
  // Canvas / triangle shaders: float pos3 | float color3 | float uv2 (stride
  // 32)
  Pos3Color3Uv2 = 0,
  // UI / stb_easy_font: float pos3 | ubyte color4 (stride 16)
  Pos3Color4U8 = 1,
};

class IMesh
{
public:
  IMesh() = default;
  IMesh(const std::vector<float>& vertexData,
        const std::vector<unsigned int>& indexData = {})
    : _vertexData(vertexData)
    , _indexData(indexData)
  {
  }
  IMesh(const std::vector<float>& vertexData)
    : _vertexData(vertexData)
  {
  }
  virtual ~IMesh() = default;
  virtual void Destroy() = 0;

  unsigned int getVertexCount() const
  {
    return static_cast<unsigned int>(_vertexData.size() / 6);
  }
  unsigned int getIndexCount() const
  {
    return static_cast<unsigned int>(_indexData.size());
  }
  unsigned int getVAOID() const { return _vaoID; }
  unsigned int getVBOID() const { return _vboID; }
  unsigned int getEBOID() const { return _eboID; }

protected:
  std::vector<float> _vertexData;
  std::vector<unsigned int> _indexData;
  unsigned int _vaoID = 0;
  unsigned int _vboID = 0;
  unsigned int _eboID = 0;
};