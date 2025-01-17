#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

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

    static bool LogError(char* message);
    static bool LogWarning(char* message);
    static bool LogInfo(char* message);
    static bool Log(char* message);

    //Wide character logging functions
    static bool LogWError(const wchar_t* message) {
        return true;
    };
    static bool LogWWarning(const wchar_t* message) {
        return true;
    };
    static bool LogWInfo(const wchar_t* message) {
        return true;
    };
    static bool LogW(const wchar_t* message) {
        return true;
    };
    /*
        All logging functions return a integer which represents the number of bytes written.
        This allows for checking for any apparent errors. Just check if the returned value is 0.
    */
   static bool initLogger();
   static std::ofstream logFileStream;

 };