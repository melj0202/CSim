//A Generic Pool Allocator
#include <vector>
#define MAX_POOL_CHUNKS 4

template <typename T>
class ChainedPoolAlloc{
    private:
        struct Chunk {
            char* data;
            Chunk* next;
        };
        Chunk* chunk;
        Chunk* chunkHead;
        size_t blockSize;
        size_t numBlocks;
        size_t numChunks;
        std::vector<T*> freeList;
    public:
        ChainedPoolAlloc(size_t numBlocks);
        ~ChainedPoolAlloc();

        T* Allocate() ;
        void Deallocate(T* block) ;
        void Clear() ;

        size_t getBlockSize() const { return blockSize; }
        size_t getNumBlocks() const { return numBlocks; }
        size_t getNumChunks() const { return numChunks; }
        size_t getBlocksFree() const { return freeList.size(); }
};