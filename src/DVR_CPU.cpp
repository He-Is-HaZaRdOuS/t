#include "Application/Application.h"


int main()
{
    Application::Initialize();
    CameraUtils::Initialize();
    Game::Initialize(Application::windowSize);

    return Application::Render();
}
