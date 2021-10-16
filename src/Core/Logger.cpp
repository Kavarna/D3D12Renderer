#include "Logger.h"

std::ofstream Logger::gOutputStream("OblivionLogs.txt");

void Logger::Init()
{
    // Maybe do something here?
}

void Logger::Close()
{
    gOutputStream.close();
    // Maybe do something here?
}
