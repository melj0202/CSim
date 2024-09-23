#include "../CellLogger.h"
#include <iostream>

int main(int argc, char** argv) {

    CellLogger::initLogger();

    CellLogger::LogError("This is a test");
    CellLogger::LogWarning("This is a test");
    CellLogger::LogInfo("This is a test");

    std::cout << std::endl << "TESTING static array strings" << std::endl;

    char str[64] = "This is a test";
    CellLogger::LogError(str);
    CellLogger::LogWarning(str);
    CellLogger::LogInfo(str);

    std::cout << std::endl << "TESTING dynamic heap array strings" << std::endl;

    char* dstr = "This is a test";
    CellLogger::LogError(str);
    CellLogger::LogWarning(str);
    CellLogger::LogInfo(str);   
}