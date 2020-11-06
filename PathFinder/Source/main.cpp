#pragma once
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include "Application.hpp"

int main(int argc, char** argv)
{
    PathFinder::Application app{ argc, argv };
    app.RunMessageLoop();
    return 0;
}