#include "Application/Application.hpp"
#include "Camera/Camera.hpp"
#include "Game/Game.hpp"

int main()
{
    Application::Initialize();
    CameraUtils::Initialize();
    Game::Initialize(Application::windowSize);

    return Application::Render();
}
