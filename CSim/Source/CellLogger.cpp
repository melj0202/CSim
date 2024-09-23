#include "CellLogger.h"
std::ofstream CellLogger::logFileStream;

bool CellLogger::LogInfo(const char* message) {
    if (std::strlen(message) == 0) return true;
    std::cout << "INFO: " << message << std::endl;
    CellLogger::logFileStream << "INFO: "<< message << std::endl;
    return true;
}

bool CellLogger::LogWarning(const char* message) {
    if (std::strlen(message) == 0) return true;
    std::cout << "WARNING: " << message << std::endl;
    CellLogger::logFileStream << "WARNING: "<< message << std::endl;
    return true;
}

bool CellLogger::LogError(const char* message) {
    if (std::strlen(message) == 0) return true;
    std::cout << "ERROR: " << message << std::endl;
    CellLogger::logFileStream << "ERROR: " << message << std::endl;
    return true;
}

bool CellLogger::Log(const char* message) {
    if (std::strlen(message) == 0) return true;
    std::cout << message << std::endl;
    CellLogger::logFileStream << message << std::endl;
    return true;
}

bool CellLogger::initLogger() {
    CellLogger::logFileStream.open("log.txt");
    return true;
}