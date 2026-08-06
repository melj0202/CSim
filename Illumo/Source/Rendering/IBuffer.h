#pragma once

class IBuffer
{
public:
  virtual ~IBuffer() = default;
  virtual void SetData(const void* data, size_t size) = 0;
};