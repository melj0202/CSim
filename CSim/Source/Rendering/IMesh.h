#pragma once 

#include <vector>
#include <iostream>

class IMesh {
public: 
    IMesh() = default;
    IMesh(const std::vector<float>& vertexData, const std::vector<unsigned int>& indexData = {}) : _vertexData(vertexData), _indexData(indexData) {}
    IMesh(const std::vector<float>& vertexData) : _vertexData(vertexData) {}
    virtual ~IMesh() = default;
    virtual void Destroy() = 0; 

    unsigned int getVertexCount() const { return _vertexData.size() / 6; }
    unsigned int getIndexCount() const { return _indexData.size(); }
    unsigned int getVAOID() const { return _vaoID; }
    unsigned int getVBOID() const { return _vboID; }
    unsigned int getEBOID() const { return _eboID; }

protected:
    std::vector<float> _vertexData; //Keep the data in CPU memory for other important features
    std::vector<unsigned int> _indexData;
    unsigned int _vaoID;
    unsigned int _vboID;
    unsigned int _eboID;

};