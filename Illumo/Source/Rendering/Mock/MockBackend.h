#pragma once
#include "Rendering/CommandQueue.h"
#include "Rendering/IBackend.h"
#include <string>
#include <vector>

// Headless IBackend for automated tests (Phase 6).
// Records enroll/create calls and snapshots the command queue on Submit.
// Does not touch OpenGL or any GPU API.
class MockBackend : public IBackend
{
public:
  struct CreateRecord
  {
    enum class Kind
    {
      Mesh,
      ShaderPaths,
      ShaderSources,
      TextureData,
      TextureFile,
      DescriptorSet
    };
    Kind kind = Kind::Mesh;
    unsigned long tableID = 0;
    size_t vertexSize = 0;
    size_t indexSize = 0;
    int width = 0;
    int height = 0;
    int channels = 0;
    MeshVertexLayout layout = MeshVertexLayout::Pos3Color3Uv2;
    bool dynamic = false;
    std::string pathOrNote;
  };

private:
  CommandQueue commandQueue;
  std::vector<RenderCommand> lastSubmitted;
  // Survives empty EndFrame re-submits (production EndFrame submits after
  // Clear).
  std::vector<RenderCommand> lastNonEmptySubmitted;
  std::vector<std::vector<RenderCommand>> submittedFrames;
  std::vector<CreateRecord> creates;
  int beginFrameCount = 0;
  int endFrameCount = 0;
  int submitCount = 0;
  int fps = 0;
  bool initialized = false;
  bool shutDown = false;

public:
  MockBackend() = default;
  ~MockBackend() override = default;

  void Initialize() override
  {
    initialized = true;
    shutDown = false;
  }

  void Shutdown() override
  {
    commandQueue.Reset();
    lastSubmitted.clear();
    creates.clear();
    shutDown = true;
  }

  void BeginFrame() override { ++beginFrameCount; }

  void EndFrame() override { ++endFrameCount; }

  void SubmitCommandQueue() override
  {
    lastSubmitted.clear();
    const size_t n = commandQueue.GetCommandCount();
    for (size_t i = 0; i < n; ++i) {
      lastSubmitted.push_back(commandQueue.GetCommand(i));
    }
    if (!lastSubmitted.empty()) {
      lastNonEmptySubmitted = lastSubmitted;
    }
    submittedFrames.push_back(lastSubmitted);
    ++submitCount;
  }

  void PushToCommandQueue(RenderCommand command) override
  {
    commandQueue.Submit(command);
  }

  void ClearCommandQueue() override { commandQueue.Reset(); }

  int getFPS() const override { return fps; }

  void setFPS(int value) { fps = value; }

  // --- Create* (record + echo tableID) ---

  unsigned long CreateMesh(const void* vertices,
                           size_t vertexSize,
                           const void* indices,
                           size_t indexSize,
                           unsigned long tableID) override
  {
    (void)vertices;
    (void)indices;
    return CreateMesh(vertices,
                      vertexSize,
                      indices,
                      indexSize,
                      tableID,
                      MeshVertexLayout::Pos3Color3Uv2,
                      false);
  }

  unsigned long CreateMesh(const void* vertices,
                           size_t vertexSize,
                           const void* indices,
                           size_t indexSize,
                           unsigned long tableID,
                           MeshVertexLayout layout,
                           bool dynamic) override
  {
    (void)vertices;
    (void)indices;
    CreateRecord rec;
    rec.kind = CreateRecord::Kind::Mesh;
    rec.tableID = tableID;
    rec.vertexSize = vertexSize;
    rec.indexSize = indexSize;
    rec.layout = layout;
    rec.dynamic = dynamic;
    creates.push_back(rec);
    return tableID;
  }

  unsigned long CreateMesh(std::string filePath, unsigned long tableID) override
  {
    CreateRecord rec;
    rec.kind = CreateRecord::Kind::Mesh;
    rec.tableID = tableID;
    rec.pathOrNote = filePath;
    creates.push_back(rec);
    return tableID;
  }

  unsigned long CreateShaderProgram(const ShaderPaths& paths,
                                    unsigned long tableID) override
  {
    CreateRecord rec;
    rec.kind = CreateRecord::Kind::ShaderPaths;
    rec.tableID = tableID;
    rec.pathOrNote = paths.vertexPath + "|" + paths.fragmentPath;
    creates.push_back(rec);
    return tableID;
  }

  unsigned long CreateShaderProgram(const ShaderSources& sources,
                                    unsigned long tableID) override
  {
    (void)sources;
    CreateRecord rec;
    rec.kind = CreateRecord::Kind::ShaderSources;
    rec.tableID = tableID;
    rec.pathOrNote = "inline_sources";
    creates.push_back(rec);
    return tableID;
  }

  unsigned long CreateTexture(const unsigned char* data,
                              const int width,
                              const int height,
                              unsigned long tableID) override
  {
    return CreateTexture(data, width, height, 4, tableID);
  }

  unsigned long CreateTexture(const unsigned char* data,
                              const int width,
                              const int height,
                              int channels,
                              unsigned long tableID) override
  {
    (void)data;
    CreateRecord rec;
    rec.kind = CreateRecord::Kind::TextureData;
    rec.tableID = tableID;
    rec.width = width;
    rec.height = height;
    rec.channels = channels;
    creates.push_back(rec);
    return tableID;
  }

  unsigned long CreateTexture(const std::string& filePath,
                              unsigned long tableID) override
  {
    CreateRecord rec;
    rec.kind = CreateRecord::Kind::TextureFile;
    rec.tableID = tableID;
    rec.pathOrNote = filePath;
    creates.push_back(rec);
    return tableID;
  }

  unsigned long CreateDescriptorSet() override
  {
    CreateRecord rec;
    rec.kind = CreateRecord::Kind::DescriptorSet;
    rec.tableID = 0;
    creates.push_back(rec);
    return 0;
  }

  // --- Inspection API for tests ---

  bool wasInitialized() const { return initialized; }
  bool wasShutdown() const { return shutDown; }
  int getBeginFrameCount() const { return beginFrameCount; }
  int getEndFrameCount() const { return endFrameCount; }
  int getSubmitCount() const { return submitCount; }

  size_t getPendingCommandCount() const
  {
    return commandQueue.GetCommandCount();
  }

  size_t getLastSubmittedCount() const { return lastSubmitted.size(); }

  const RenderCommand& getLastSubmitted(size_t index) const
  {
    return lastSubmitted[index];
  }

  CommandType getLastSubmittedType(size_t index) const
  {
    return lastSubmitted[index].commandType;
  }

  // Prefer this after a full BeginFrame/RenderScene/EndFrame cycle.
  size_t getLastNonEmptySubmittedCount() const
  {
    return lastNonEmptySubmitted.size();
  }

  const RenderCommand& getLastNonEmptySubmitted(size_t index) const
  {
    return lastNonEmptySubmitted[index];
  }

  CommandType getLastNonEmptySubmittedType(size_t index) const
  {
    return lastNonEmptySubmitted[index].commandType;
  }

  size_t getSubmittedFrameCount() const { return submittedFrames.size(); }

  const std::vector<RenderCommand>& getSubmittedFrame(size_t frameIndex) const
  {
    return submittedFrames[frameIndex];
  }

  size_t countNonEmptyOfType(CommandType type) const
  {
    size_t n = 0;
    for (size_t i = 0; i < lastNonEmptySubmitted.size(); ++i) {
      if (lastNonEmptySubmitted[i].commandType == type) {
        ++n;
      }
    }
    return n;
  }

  bool nonEmptyStartsWith(const CommandType* types, size_t typeCount) const
  {
    if (typeCount > lastNonEmptySubmitted.size()) {
      return false;
    }
    for (size_t i = 0; i < typeCount; ++i) {
      if (lastNonEmptySubmitted[i].commandType != types[i]) {
        return false;
      }
    }
    return true;
  }

  size_t getCreateCount() const { return creates.size(); }

  const CreateRecord& getCreate(size_t index) const { return creates[index]; }

  // Count how many submitted commands match type.
  size_t countSubmittedOfType(CommandType type) const
  {
    size_t n = 0;
    for (size_t i = 0; i < lastSubmitted.size(); ++i) {
      if (lastSubmitted[i].commandType == type) {
        ++n;
      }
    }
    return n;
  }

  // True if submitted sequence starts with the given type list (prefix match).
  bool submittedStartsWith(const CommandType* types, size_t typeCount) const
  {
    if (typeCount > lastSubmitted.size()) {
      return false;
    }
    for (size_t i = 0; i < typeCount; ++i) {
      if (lastSubmitted[i].commandType != types[i]) {
        return false;
      }
    }
    return true;
  }

  void resetCounters()
  {
    beginFrameCount = 0;
    endFrameCount = 0;
    submitCount = 0;
    lastSubmitted.clear();
    lastNonEmptySubmitted.clear();
    submittedFrames.clear();
    commandQueue.Reset();
    creates.clear();
  }
};
