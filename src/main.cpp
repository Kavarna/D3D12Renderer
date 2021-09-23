#include "Application.h"


int main(int argc, char *argv[])
{
    Logger::Init();
    Application app;
    CHECK(app.Init(GetModuleHandle(NULL)), 0, "Cannot initialize application");
    app.Run();
    Logger::Close();
    return 0;
}