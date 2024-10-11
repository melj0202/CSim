//
// Created by gravi on 10/6/2024.
//

/*
Command Line Arguements:
//
 "-ww": Render window width
 "-wh": Render window height
 "-cw": Cell canvas width
 "-ch": Cell canvas height
 "--help": Displays the help message and then kills the program.
 "--version": Displays extended version information and then kills the program.
*/

#pragma once


/*
  These functions serve as the cross platform
 */

extern void ParseCommandLine(int argc, char **argv);

extern bool StringIsDigit(char *str);
extern bool StringisModeString(char *str);

