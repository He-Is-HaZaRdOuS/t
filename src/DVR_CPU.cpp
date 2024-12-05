#include "Application/Application.h"

int cpu_main()
{
    Application::Initialize();
    CameraUtils::Initialize();
    Game::Initialize(Application::windowSize);

    return Application::Render();
}
