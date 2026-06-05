#include "PoolAlloc.h"

template <typename T>
ChainedPoolAlloc<T>::ChainedPoolAlloc(size_t numBlocks) {
    this->numChunks = 0;
    this->blockSize = sizeof(T);
    this->numBlocks = numBlocks;
    this->chunk = new Chunk();
    this->chunkHead = new Chunk();
    this->chunk->data = new char[blockSize * numBlocks];
    this->chunk->next = nullptr;
    for (size_t i = 0; i < numBlocks; i++) {
        this->freeList.push_back(reinterpret_cast<T*>(this->chunk->data + (i * blockSize)));
    }
    this->numChunks++;
    chunkHead = chunk;
}
template <typename T>
ChainedPoolAlloc<T>::~ChainedPoolAlloc() {
    Chunk* currentBlock = chunkHead;
    while (currentBlock != nullptr) {
        Chunk* nextBlock = currentBlock->next;
        delete[] currentBlock->data;
        delete currentBlock;
        currentBlock = nextBlock;
    }
}


template <typename T>
T* ChainedPoolAlloc<T>::Allocate() {
    if (freeList.empty()) {
        if(numChunks == MAX_POOL_CHUNKS) {
            return nullptr;
        }
        this->chunk->next = new Chunk;
        this->chunk->next->data = new char[blockSize * numBlocks];
        this->chunk->next->next = nullptr;
        this->chunk = this->chunk->next;
        for (size_t i = 0; i < numBlocks; i++) {
            this->freeList.push_back(reinterpret_cast<T*>(this->chunk->data + (i * blockSize)));
        }
        this->numChunks++;
    }
    T* block = freeList.back();
    freeList.pop_back();
    return block;
}

template <typename T>
void ChainedPoolAlloc<T>::Deallocate(T* block) {
    if (block == nullptr) {
        return;
    }
    freeList.push_back(block);
}

template <typename T>
void ChainedPoolAlloc<T>::Clear() {
    freeList.clear();
    Chunk* currentBlock = chunkHead;
    while (currentBlock != nullptr) {
        Chunk* nextBlock = currentBlock->next;
        delete[] currentBlock->data;
        delete currentBlock;
        currentBlock = nextBlock;
    }
    chunkHead = new Chunk();
    chunkHead->data = new char[blockSize * numBlocks];
    chunkHead->next = nullptr;
    chunk = chunkHead;
    numChunks = 1;
}