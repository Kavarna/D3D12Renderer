#include "Oblivion.h"
#include "Application.h"

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    google::SetLogFilenameExtension(".log");
    google::SetLogDestination(0, "./Logs/Oblivion");
    
    
    Application app;
    app.Init(GetModuleHandle(NULL));
    app.Run();
    return 0;
}