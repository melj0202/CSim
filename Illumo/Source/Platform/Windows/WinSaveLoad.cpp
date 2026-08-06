//
// Created by gravi on 10/6/2024.
//
#include "SaveLoad.h"
#include <windows.h>

std::string
SaveLoad::GetLoadLocation()
{
  OPENFILENAMEA ofn;

  char szFile[256] = "myCanvas.illumo\0";

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Illumo File Format (.illumo)\0*.ILLUMO\0";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (!GetOpenFileNameA(&ofn))
    return "";
  else
    return std::string{ szFile };
}

std::string
SaveLoad::GetSaveLocation()
{
  OPENFILENAMEA ofn;

  char szFile[256] = "MyCanvas.illumo\0";

  ZeroMemory(&ofn, sizeof(ofn));
  ofn.lStructSize = sizeof(ofn);
  ofn.hwndOwner = NULL;
  ofn.lpstrFile = szFile;
  ofn.nMaxFile = sizeof(szFile);
  ofn.lpstrFilter = "Illumo File Format (.illumo)\0*.ILLUMO";
  ofn.nFilterIndex = 1;
  ofn.lpstrFileTitle = NULL;
  ofn.nMaxFileTitle = 0;
  ofn.lpstrInitialDir = NULL;
  ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

  if (!GetSaveFileNameA(&ofn))
    return "";
  else
    return std::string{ szFile };
}