#include "CellMain.h"
#include "EnvVars.h"
#include "ServiceLocator.h"
#include "SysCmdLine.h"
#include <iostream>
#include <string>

int
main(int argc, char** argv)
{
  std::string mode;

  // Parse command line args
  if (argc < 2) {
    mode = "GAME_OF_LIFE";
  } else {
    SysCmdLine::ParseCommandLine(argc, argv);
    if (!SysCmdLine::StringisModeString(argv[1])) {
      std::cout << "ERROR: Missing Ruleset" << std::endl;
      std::exit(0);
    }
    mode = argv[1];
  }

  // Call the main game initialization loop
  CellMain(mode);
  return 0;
}
