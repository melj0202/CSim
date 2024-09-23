#pragma once

#define MAX_CHARS_PER_LINE 1024
#define MAX_CMD_HISTORY 256

class CellCommandLine {
	static char line[MAX_CHARS_PER_LINE];

	static char history[MAX_CHARS_PER_LINE][MAX_CMD_HISTORY];

	static bool AllocateCommandLine();

	static void getCommandLine();
};