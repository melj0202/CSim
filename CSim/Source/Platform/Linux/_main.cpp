#include <string>
#include "CellMain.h"
#include "SysCmdLine.h"
#include "EnvVars.h"
#include "ServiceLocator.h"

int main(int argc, char** argv) {
	//Parse command line args
	ServiceLocator::provide("EnvVars", new EnvVars());
	if (argc < 2) {
		ServiceLocator::get<EnvVars>("EnvVars")->setVar("ModeString", "GAME_OF_LIFE");
	} else {
		SysCmdLine::ParseCommandLine(argc, argv);
		if (!SysCmdLine::StringisModeString(argv[1])) {
			std::cout << "ERROR: Missing Ruleset" << std::endl;
			std::exit(0);
		}
		ServiceLocator::get<EnvVars>("EnvVars")->setVar("ModeString", argv[1]);
	}
	CellMain(ServiceLocator::get<EnvVars>("EnvVars")->getVar("ModeString").value);
	return 0;
}