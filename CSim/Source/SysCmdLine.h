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

#ifndef _SYSCMDLINE_H_
#define _SYSCMDLINE_H_

/*
  These functions serve as the cross platform
 */
class SysCmdLine {
 public:

 static void ParseCommandLine(int argc, char **argv);

 static bool StringIsDigit(char *str);
 static bool StringisModeString(char *str);
};
#endif