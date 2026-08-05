#pragma once 
#include <string>
#include <array>

class ITexture {
public:
    ITexture() = default;
    ITexture(std::string path) {}
    virtual ~ITexture() = default;
    virtual void Bind(unsigned int slot) const = 0;
    virtual unsigned int getID() const = 0;
    virtual std::array<int, 2> getSize() const = 0;
    virtual void Destroy() = 0;

protected:
    unsigned int _textureID;
    std::array<int, 2> _size;
};