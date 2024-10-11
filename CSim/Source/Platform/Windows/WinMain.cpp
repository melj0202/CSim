#include <string>
#include "../../CellMain.h"
#include "../../SysCmdLine.h"

int main(int argc, char** argv) {
    
	std::string mode;

	//Parse command line args

	if (argc < 2) mode = "GAME_OF_LIFE";
	else {
		ParseCommandLine(argc, argv);
		if (!StringisModeString(argv[1])) {
			std::cout << "ERROR: Missing Ruleset" << std::endl;
			std::exit(0);
		}
		mode = argv[1];
	}
    CellMain(mode);
    return 0;
}