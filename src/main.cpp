#include "Application.h"


int main(int argc, char *argv[])
{
    Logger::Init();
    Application app;
    app.Init(GetModuleHandle(NULL));
    app.Run();
    Logger::Close();
    return 0;
}