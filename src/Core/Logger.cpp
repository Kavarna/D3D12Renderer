#include "Logger.h"

fmt::ostream Logger::gOutputStream = fmt::output_file("OblivionLogs.txt");

void Logger::Init()
{
    // Maybe do something here?
}

void Logger::Close()
{
    gOutputStream.close();
    // Maybe do something here?
}
