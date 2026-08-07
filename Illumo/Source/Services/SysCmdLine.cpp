//
// Created by gravi on 10/6/2024.
//
#include "SysCmdLine.h"
#include "CellMain.h"
#include "Foundation/BuildInfo.h"
#include "IEnvVars.h"
#include <iostream>

void
SysCmdLine::ParseCommandLine(int argc, char** argv, IEnvVars* envVars)
{
  if (argc <= 1 || argv == nullptr || envVars == nullptr) {
    return;
  }

  for (int i = 1; i < argc; ++i) {

    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      // Print out a helpful message dialog and then exit the program
      // completely.
      std::cout << "Illumo Cell Automata Simulator\nUsage: illumo.exe RULESET "
                   "[OPTION] ... [FILE] ...\n\n"
                << "Options:\n-ww\t The render window width in pixels\n"
                << "-wh\t The render window height in pixels\n"
                << "-cw\t The cell colony width in pixels\n"
                << "-ch\t The cell colony height in pixels\n"
                << "-h, --help\t Print out this help message\n"
                << "-v, --version\t Print out version information for this "
                   "application.\n\n"
                << "Rulesets:\nGAME_OF_LIFE\t\t Conway's Game of Life\n"
                << "BRIANS_BRAIN\t\t Brians Brain\n"
                << "LIFE_WITHOUT_DEATH\t Life Without Death\n"
                << "HIGHLIFE\t\t Similar to Game of Life\n"
                << "SEEDS\t\t\t Seeds\n"
                << "DAY_AND_NIGHT\t\t Day and Night\n"
                << "WIREWORLD\t\t Wireworld (circuit CA)\n";
      std::exit(0);
      return;
    }
    if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
      std::cout << "Illumo Cell Automata Simulator\nVersion: " << VERSION_NUMBER
                << "\nBuild Date: " << BUILD_DATE_SHORT << " "
                << BUILD_TIMESTAMP << std::endl;
      std::exit(0);
      return;
    }
    if (!strcmp(argv[i], "-ww")) {
      if (i + 1 >= argc || argv[i + 1] == nullptr ||
          !StringIsDigit(argv[i + 1])) {
        std::cout << "ERROR: option '-ww' has invalid type!\n";
        std::exit(1);
      } else {
        envVars->setVar("WinX", argv[i + 1]);
        ++i;
      }
      continue;
    }
    if (!strcmp(argv[i], "-wh")) {
      if (i + 1 >= argc || argv[i + 1] == nullptr ||
          !StringIsDigit(argv[i + 1])) {
        std::cout << "ERROR: option '-wh' has invalid type!\n";
        std::exit(1);
      } else {
        envVars->setVar("WinY", argv[i + 1]);
        ++i;
      }
      continue;
    }
    if (!strcmp(argv[i], "-cw")) {
      if (i + 1 >= argc || argv[i + 1] == nullptr ||
          !StringIsDigit(argv[i + 1])) {
        std::cout << "ERROR: option '-cw' has invalid type!\n";
        std::exit(1);
      } else {
        envVars->setVar("CanvasX", argv[i + 1]);
        ++i;
      }
      continue;
    }
    if (!strcmp(argv[i], "-ch")) {
      if (i + 1 >= argc || argv[i + 1] == nullptr ||
          !StringIsDigit(argv[i + 1])) {
        std::cout << "ERROR: option '-ch' has invalid type!\n";
        std::exit(1);
      } else {
        envVars->setVar("CanvasY", argv[i + 1]);
        ++i;
      }
      continue;
    }
  }
}

bool
SysCmdLine::StringIsDigit(char* str)
{
  long size = strlen(str);
  for (int i = 0; i < size; i++) {
    if (!isdigit(str[i])) {
      return false;
    }
  }
  return true;
}
bool
SysCmdLine::StringisModeString(char* str)
{
  long size = strlen(str);
  for (int i = 0; i < size; i++) {
    if (!isalpha(str[i])) {
      if (str[i] == '_')
        continue; // spare the '_' character
      return false;
    }
  }
  return true;
}
