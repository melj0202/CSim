#define _CRTDBG_MAP_ALLOC
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINSOCKAPI_

#include <winsock2.h>
#include <ws2tcpip.h>

#include <windows.h>
#include <crtdbg.h>
#endif

#include "CellMain.h"

int main(int argc, char** argv)
{

#ifdef _WIN32
	_CrtSetDbgFlag(
		_CRTDBG_ALLOC_MEM_DF |
		_CRTDBG_LEAK_CHECK_DF
	);

	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif

	CellMain(argc, argv);
	return 0;
}