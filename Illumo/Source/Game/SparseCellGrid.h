#pragma once

class SparseCellGrid
{
public:
  static const unsigned char BackgroundState = 1;
  static const unsigned char CountedNeighborState = 0;
  unsigned char getCell(const CellAddress& address) const;
  bool setCell(const CellAddress& address, unsigned char state);
  void clear();
  bool advance(const RuleSet& ruleSet);
  std::uint64_t getRevision() const;
  std::size_t getAllocatedChunkCount() const;
};