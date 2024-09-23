#pragma once
#include <iostream>
#include <fstream>
#include <string>

/*
    This class describes a logger class that writes messages to a file, 
    and to a terminal if there is one present in the build 
    (aka if it is a debug build)
*/
 class CellLogger {
public:
    CellLogger();
    ~CellLogger();
    static bool LogError(const char* message);
    static bool LogWarning(const char* message);
    static bool LogInfo(const char* message);
    static bool Log(const char* message);

    //Wide character logging functions
    static bool LogWError(const wchar_t* message) {/*DO NOTHING FOR NOW*/};
    static bool LogWWarning(const wchar_t* message) {/*DO NOTHING FOR NOW*/};
    static bool LogWInfo(const wchar_t* message) {/*DO NOTHING FOR NOW*/};
    static bool LogW(const wchar_t* message) {/*DO NOTHING FOR NOW*/};
    /*
        All logging functions return a integer which represents the number of bytes written.
        This allows for checking for any apparent errors. Just check if the returned value is 0.
    */
   static bool initLogger();
   static std::ofstream logFileStream;

 };