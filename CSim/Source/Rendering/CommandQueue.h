#pragma once 
#include "RenderCommand.h"
#include "Util/ArrayQueue.h"

class CommandQueue {
    private:
    static const size_t MAX_DRAW_COMMANDS = 2048;
    ArrayQueue<RenderCommand>* commandQueue;
    size_t m_CommandCount = 0;
    
    public:
    CommandQueue() {
        commandQueue = new ArrayQueue<RenderCommand>(MAX_DRAW_COMMANDS);
    }
    ~CommandQueue() {
        delete commandQueue;
    }
    void Reset() { 
        m_CommandCount = 0;
    }
    void Submit(RenderCommand command) {
        if (m_CommandCount < MAX_DRAW_COMMANDS) {
            (*commandQueue)[m_CommandCount] = command;
            m_CommandCount++;
        }
    }
    size_t GetCommandCount() const { return m_CommandCount; }
    RenderCommand& GetCommand(size_t index) { return (*commandQueue)[index]; }
};