#include "IBuffer.h"
#include <GL/glew.h>

class GLBuffer : public IBuffer
{
private:
  unsigned int m_VaoID = 0; // The state container (Remembers attribute links)
  unsigned int m_VboID =
    0; // The raw vertex data bucket (Positions, Colors, UVs)
  unsigned int m_IboID =
    0; // The raw index data bucket (The order to connect triangles)

  size_t m_VertexCount = 0;
  size_t m_IndexCount = 0;

public:
  GLBuffer(const void* vertexData,
           size_t vertexSize,
           const void* indexData,
           size_t indexSize)
  {
    m_VertexCount = vertexSize / 28; // (Assuming a 28-byte vertex struct)
    m_IndexCount = indexSize / sizeof(unsigned int);

    // 1. Generate and bind the VAO first
    glGenVertexArrays(1, &m_VaoID);
    glBindVertexArray(m_VaoID);

    // 2. Generate, bind, and fill the VBO (Vertices)
    glGenBuffers(1, &m_VboID);
    glBindBuffer(GL_ARRAY_BUFFER, m_VboID);
    glBufferData(GL_ARRAY_BUFFER, vertexSize, vertexData, GL_STATIC_DRAW);

    // 3. Set up the Attribute Pointers (The rulebook)
    // (glVertexAttribPointer calls go here, telling OpenGL how to read the VBO)

    // 4. Generate, bind, and fill the IBO (Indices)
    // CRITICAL NOTE: When a VAO is bound, it permanently remembers the IBO
    // binding!
    if (indexData != nullptr) {
      glGenBuffers(1, &m_IboID);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_IboID);
      glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, indexSize, indexData, GL_STATIC_DRAW);
    }

    // 5. Unbind the VAO so we don't accidentally modify it later
    glBindVertexArray(0);
  }

  ~GLBuffer() override
  {
    // Clean up all GPU allocations cleanly when this C++ object dies
    if (m_IboID != 0)
      glDeleteBuffers(1, &m_IboID);
    if (m_VboID != 0)
      glDeleteBuffers(1, &m_VboID);
    if (m_VaoID != 0)
      glDeleteVertexArrays(1, &m_VaoID);
  }

  void SetData(const void* data, size_t size) override
  {
    glBindBuffer(GL_ARRAY_BUFFER, m_VboID);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_DYNAMIC_DRAW);
  }

  // Getters for your OpenGL Command List to read during Submit()
  unsigned int GetVAOID() const { return m_VaoID; }
  size_t GetIndexCount() const { return m_IndexCount; }
  bool HasIndices() const { return m_IboID != 0; }
};