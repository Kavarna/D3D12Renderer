#include <glog/logging.h>
#include "Result.h"

int main(int argc, char *argv[])
{
    google::InitGoogleLogging(argv[0]);
    google::SetLogFilenameExtension(".log");
    google::SetLogDestination(0, "./Logs/Oblivion");
    
    LOG(INFO) << "Hello world!1";
    LOG(WARNING) << "Hello world!1";
    LOG(ERROR) << "Hello world!1";
}