#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include "loader_ui.h"

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int)
{
    return LoaderUI::Run();
}
