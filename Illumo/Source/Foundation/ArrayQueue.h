#pragma once

template<typename T>
class ArrayQueue
{
public:
  ArrayQueue(size_t capacity)
    : size(0)
    , capacity(capacity)
    , head(0)
    , tail(0)
  {
    data = new T[capacity];
  }
  ~ArrayQueue() { delete[] data; };

  inline void enqueue(const T& item)
  {
    if (size == capacity) {
      return;
    }
    data[tail] = item;
    tail = (tail + 1) % capacity;
    size++;
  };

  inline void dequeue(T& item)
  {
    if (size == 0) {
      return;
    }
    item = data[head];
    head = (head + 1) % capacity;
    size--;
  };

  inline bool isFull() const { return size == capacity; };

  inline bool isEmpty() const { return size == 0; };

  inline size_t getSize() const { return size; };

  inline size_t getCapacity() const { return capacity; };

  T& operator[](size_t index) { return data[index]; }

  const T& operator[](size_t index) const { return data[index]; }

private:
  T* data;
  size_t size;
  size_t capacity;
  size_t head;
  size_t tail;
};