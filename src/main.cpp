#include "Core/Application.h"

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    google::SetLogFilenameExtension(".log");
    google::SetLogDestination(0, "./Logs/Oblivion");
    
    
    Application app;
    CHECK(app.Init(GetModuleHandle(NULL))) << "Unable to initialize Application";
    app.Run();
    return 0;
}