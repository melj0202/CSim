#pragma once

#include <string>

struct SaveLoadDialogSpec
{
  std::string fileDescription{ "Illumo File Format" };
  std::string defaultFilename{ "MyCanvas.illumo" };
  std::string extensionPattern{ "*.ILLUMO" };
};

class SaveLoad
{
public:
  static std::string GetLoadLocation(
    const SaveLoadDialogSpec& specification = SaveLoadDialogSpec{});
  static std::string GetSaveLocation(
    const SaveLoadDialogSpec& specification = SaveLoadDialogSpec{});
};
