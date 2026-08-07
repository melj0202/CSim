#pragma once
#include "Foundation/ArrayQueue.h"
#include "RenderCommand.h"
#include "Services/Logger.h"

// Fixed-capacity per-frame command buffer. Overflow drops new commands and
// logs once per Reset() interval so a runaway emitter is visible without
// spamming every discarded token (D-R12).
class CommandQueue
{
private:
  static const size_t MAX_DRAW_COMMANDS = 2048;
  ArrayQueue<RenderCommand>* commandQueue;
  size_t m_CommandCount = 0;
  size_t m_DroppedThisFrame = 0;
  size_t m_TotalDropped = 0;
  bool m_OverflowLoggedThisFrame = false;

public:
  CommandQueue()
  {
    commandQueue = new ArrayQueue<RenderCommand>(MAX_DRAW_COMMANDS);
  }
  ~CommandQueue() { delete commandQueue; }

  void Reset()
  {
    m_CommandCount = 0;
    m_DroppedThisFrame = 0;
    m_OverflowLoggedThisFrame = false;
  }

  void Submit(RenderCommand command)
  {
    if (m_CommandCount < MAX_DRAW_COMMANDS) {
      (*commandQueue)[m_CommandCount] = command;
      m_CommandCount++;
      return;
    }

    m_DroppedThisFrame++;
    m_TotalDropped++;
    if (!m_OverflowLoggedThisFrame) {
      m_OverflowLoggedThisFrame = true;
      Logger::LogError(
        "CommandQueue full (2048 commands); dropping overflow tokens until "
        "Clear/Reset. Raise capacity only after profiling the emitter.");
    }
  }

  size_t GetCommandCount() const { return m_CommandCount; }
  size_t GetCapacity() const { return MAX_DRAW_COMMANDS; }
  size_t GetDroppedThisFrame() const { return m_DroppedThisFrame; }
  size_t GetTotalDropped() const { return m_TotalDropped; }
  bool HasOverflowedThisFrame() const { return m_DroppedThisFrame > 0; }
  RenderCommand& GetCommand(size_t index) { return (*commandQueue)[index]; }
};
