#include "CellLogger.h"
std::ofstream CellLogger::logFileStream;

bool CellLogger::LogInfo(const char* message) {
    if (std::strlen(message) == 0) return true;
#ifndef NDEBUG
    printf("\x1B[34mINFO\033[0m: %s\n", message);
#endif
    CellLogger::logFileStream << "INFO: "<< message << std::endl;
    return true;
}

bool CellLogger::LogWarning(const char* message) {
    if (std::strlen(message) == 0) return true;
#ifndef NDEBUG
    printf("\x1B[33mWARNING\033[0m: %s\n", message);
#endif
    CellLogger::logFileStream << "WARNING: "<< message << std::endl;
    return true;
}

bool CellLogger::LogError(const char* message) {
    if (std::strlen(message) == 0) return true;
#ifndef NDEBUG
    printf("\x1B[31mERROR\033[0m: %s\n", message);
#endif
    CellLogger::logFileStream << "ERROR: " << message << std::endl;
    return true;
}

bool CellLogger::Log(const char* message) {
    if (std::strlen(message) == 0) return true;
#ifndef NDEBUG
    std::cout << message << std::endl;
#endif
    CellLogger::logFileStream << message << std::endl;
    return true;
}

bool CellLogger::LogInfo(char* message) {
    if (std::strlen(message) == 0) return true;
#ifndef NDEBUG
    printf("\x1B[34mINFO\033[0m: %s\n", message);
#endif
    CellLogger::logFileStream << "INFO: " << message << std::endl;
    return true;
}

bool CellLogger::LogWarning(char* message) {
    if (std::strlen(message) == 0) return true;
#ifndef NDEBUG
    printf("\x1B[33mWARNING\033[0m: %s\n", message);
#endif
    CellLogger::logFileStream << "WARNING: " << message << std::endl;
    return true;
}

bool CellLogger::LogError(char* message) {
    if (std::strlen(message) == 0) return true;
#ifndef NDEBUG
    printf("\x1B[31mERROR\033[0m: %s\n", message);
#endif
    CellLogger::logFileStream << "ERROR: " << message << std::endl;
    return true;
}

bool CellLogger::Log(char* message) {
    if (std::strlen(message) == 0) return true;
#ifndef NDEBUG
    std::cout << message << std::endl;
#endif
    CellLogger::logFileStream << message << std::endl;
    return true;
}


bool CellLogger::initLogger() {
    CellLogger::logFileStream.open("log.txt");
    return true;
}

