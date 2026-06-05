#include "Logger.h"
#include <iostream>

int main(int argc, char** argv) {

    Logger::initLogger();

    Logger::LogError("This is a test");
    Logger::LogWarning("This is a test");
    Logger::LogInfo("This is a test");

    std::cout << std::endl << "TESTING static array strings" << std::endl;

    char str[64] = "This is a test";
    Logger::LogError(str);
    Logger::LogWarning(str);
    Logger::LogInfo(str);

    std::cout << std::endl << "TESTING dynamic heap array strings" << std::endl;

    char* dstr = "This is a test";
    Logger::LogError(str);
    Logger::LogWarning(str);
    Logger::LogInfo(str);   
}